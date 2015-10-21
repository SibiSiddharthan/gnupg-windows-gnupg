/* dns-stuff.c - DNS related code including CERT RR (rfc-4398)
 * Copyright (C) 2005, 2006, 2009 Free Software Foundation, Inc.
 * Copyright (C) 2005, 2006, 2009, 2015 Werner Koch
 *
 * This file is part of GnuPG.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either
 *
 *   - the GNU Lesser General Public License as published by the Free
 *     Software Foundation; either version 3 of the License, or (at
 *     your option) any later version.
 *
 * or
 *
 *   - the GNU General Public License as published by the Free
 *     Software Foundation; either version 2 of the License, or (at
 *     your option) any later version.
 *
 * or both in parallel, as here.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <sys/types.h>
#ifdef HAVE_W32_SYSTEM
# ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
# endif
# include <windows.h>
#else
# include <netinet/in.h>
# include <arpa/nameser.h>
# include <resolv.h>
# include <netdb.h>
#endif
#include <string.h>
#ifdef USE_ADNS
# include <adns.h>
#endif

#include "util.h"
#include "host2net.h"
#include "dns-stuff.h"

/* Not every installation has gotten around to supporting CERTs
   yet... */
#ifndef T_CERT
# define T_CERT 37
#endif

/* ADNS has no support for CERT yet. */
#define my_adns_r_cert 37

/* If set Tor mode shall be used.  */
static int tor_mode;

/* Sets the module in Tor mode.  Returns 0 is this is possible or an
   error code.  */
gpg_error_t
enable_dns_tormode (void)
{
#if defined(USE_DNS_CERT) && defined(USE_ADNS)
# if HAVE_ADNS_IF_TORMODE
   tor_mode = 1;
   return 0;
# endif
#endif
  return gpg_error (GPG_ERR_NOT_IMPLEMENTED);
}

/* Free an addressinfo linked list as returned by resolve_dns_name.  */
void
free_dns_addrinfo (dns_addrinfo_t ai)
{
  while (ai)
    {
      dns_addrinfo_t next = ai->next;
      xfree (ai);
      ai = next;
    }
}


/* Resolve a name using the standard system function.  */
static gpg_error_t
resolve_name_standard (const char *name, unsigned short port,
                       int want_family, int want_socktype,
                       dns_addrinfo_t *r_dai, char **r_canonname)
{
  gpg_error_t err = 0;
  dns_addrinfo_t daihead = NULL;
  dns_addrinfo_t dai;
  struct addrinfo *aibuf = NULL;
  struct addrinfo hints, *ai;
  char portstr[21];
  int ret;

  *r_dai = NULL;
  if (r_canonname)
    *r_canonname = NULL;

  memset (&hints, 0, sizeof hints);
  hints.ai_family = want_family;
  hints.ai_socktype = want_socktype;
  if (r_canonname)
    hints.ai_flags = AI_CANONNAME;

  if (port)
    snprintf (portstr, sizeof portstr, "%hu", port);
  else
    *portstr = 0;

  /* We can't use the the AI_IDN flag because that does the conversion
     using the current locale.  However, GnuPG always used UTF-8.  To
     support IDN we would need to make use of the libidn API.  */
  ret = getaddrinfo (name, *portstr? portstr : NULL, &hints, &aibuf);
  if (ret)
    {
      aibuf = NULL;
      switch (ret)
        {
        case EAI_AGAIN:     err = gpg_error (GPG_ERR_EAGAIN); break;
        case EAI_BADFLAGS:  err = gpg_error (GPG_ERR_INV_FLAG); break;
        case EAI_FAIL:      err = gpg_error (GPG_ERR_SERVER_FAILED); break;
        case EAI_MEMORY:    err = gpg_error (GPG_ERR_ENOMEM); break;
        case EAI_NODATA:    err = gpg_error (GPG_ERR_NO_DATA); break;
        case EAI_NONAME:    err = gpg_error (GPG_ERR_NO_NAME); break;
        case EAI_SERVICE:   err = gpg_error (GPG_ERR_NOT_SUPPORTED); break;
        case EAI_ADDRFAMILY:err = gpg_error (GPG_ERR_EADDRNOTAVAIL); break;
        case EAI_FAMILY:    err = gpg_error (GPG_ERR_EAFNOSUPPORT); break;
        case EAI_SOCKTYPE:  err = gpg_error (GPG_ERR_ESOCKTNOSUPPORT); break;
        case EAI_SYSTEM:    err = gpg_error_from_syserror (); break;
        default:            err = gpg_error (GPG_ERR_UNKNOWN_ERRNO); break;
        }
      goto leave;
    }

  if (r_canonname && aibuf && aibuf->ai_canonname)
    {
      *r_canonname = xtrystrdup (aibuf->ai_canonname);
      if (!*r_canonname)
        {
          err = gpg_error_from_syserror ();
          goto leave;
        }
    }

  for (ai = aibuf; ai; ai = ai->ai_next)
    {
      if (ai->ai_family != AF_INET6 && ai->ai_family != AF_INET)
        continue;

      dai = xtrymalloc (sizeof *dai + ai->ai_addrlen - 1);
      dai->family = ai->ai_family;
      dai->socktype = ai->ai_socktype;
      dai->protocol = ai->ai_protocol;
      dai->addrlen = ai->ai_addrlen;
      memcpy (dai->addr, ai->ai_addr, ai->ai_addrlen);
      dai->next = daihead;
      daihead = dai;
    }

 leave:
  if (aibuf)
    freeaddrinfo (aibuf);
  if (err)
    {
      if (r_canonname)
        {
          xfree (*r_canonname);
          *r_canonname = NULL;
        }
      free_dns_addrinfo (daihead);
    }
  else
    *r_dai = daihead;
  return err;
}


/* This a wrapper around getaddrinfo with slighly different semantics.
   NAME is the name to resolve.
   PORT is the requested port or 0.
   WANT_FAMILY is either 0 (AF_UNSPEC), AF_INET6, or AF_INET4.
   WANT_SOCKETTYPE is either SOCK_STREAM or SOCK_DGRAM.

   On success the result is stored in a linked list with the head
   stored at the address R_AI; the caller must call gpg_addrinfo_free
   on this.  If R_CANONNAME is not NULL the official name of the host
   is stored there as a malloced string; if that name is not available
   NULL is stored.  */
gpg_error_t
resolve_dns_name (const char *name, unsigned short port,
                  int want_family, int want_socktype,
                  dns_addrinfo_t *r_ai, char **r_canonname)
{
#ifdef USE_ADNS_disabled_for_now
  return resolve_name_adns (name, port, want_family, want_socktype,
                            r_ai, r_canonname);
#else
  return resolve_name_standard (name, port, want_family, want_socktype,
                                r_ai, r_canonname);
#endif
}


/* Returns 0 on success or an error code.  If a PGP CERT record was
   found, the malloced data is returned at (R_KEY, R_KEYLEN) and
   the other return parameters are set to NULL/0.  If an IPGP CERT
   record was found the fingerprint is stored as an allocated block at
   R_FPR and its length at R_FPRLEN; an URL is is allocated as a
   string and returned at R_URL.  If WANT_CERTTYPE is 0 this function
   returns the first CERT found with a supported type; it is expected
   that only one CERT record is used.  If WANT_CERTTYPE is one of the
   supported certtypes only records with this certtype are considered
   and the first found is returned.  (R_KEY,R_KEYLEN) are optional. */
gpg_error_t
get_dns_cert (const char *name, int want_certtype,
              void **r_key, size_t *r_keylen,
              unsigned char **r_fpr, size_t *r_fprlen, char **r_url)
{
#ifdef USE_DNS_CERT
#ifdef USE_ADNS
  gpg_error_t err;
  adns_state state;
  adns_answer *answer = NULL;
  unsigned int ctype;
  int count;

  if (r_key)
    *r_key = NULL;
  if (r_keylen)
    *r_keylen = 0;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  if (tor_mode? adns_init_strcfg (&state, adns_if_noerrprint|adns_if_tormode,
                                  NULL, "nameserver 8.8.8.8")
      /*    */: adns_init (&state, adns_if_noerrprint, NULL))
    {
      err = gpg_err_make (default_errsource, gpg_err_code_from_syserror ());
      log_error ("error initializing adns: %s\n", strerror (errno));
      return err;
    }

  if (adns_synchronous (state, name,
                        (adns_r_unknown
                         | (want_certtype < DNS_CERTTYPE_RRBASE
                            ? my_adns_r_cert
                            : (want_certtype - DNS_CERTTYPE_RRBASE))),
                        adns_qf_quoteok_query, &answer))
    {
      err = gpg_err_make (default_errsource, gpg_err_code_from_syserror ());
      /* log_error ("DNS query failed: %s\n", strerror (errno)); */
      adns_finish (state);
      return err;
    }
  if (answer->status != adns_s_ok)
    {
      /* log_error ("DNS query returned an error: %s (%s)\n", */
      /*            adns_strerror (answer->status), */
      /*            adns_errabbrev (answer->status)); */
      err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);
      goto leave;
    }

  err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);
  for (count = 0; count < answer->nrrs; count++)
    {
      int datalen = answer->rrs.byteblock[count].len;
      const unsigned char *data = answer->rrs.byteblock[count].data;

      /* First check for our generic RR hack.  */
      if (datalen
          && want_certtype >= DNS_CERTTYPE_RRBASE
          && ((want_certtype - DNS_CERTTYPE_RRBASE)
              == (answer->type & ~adns_r_unknown)))
        {
          /* Found the requested record - return it.  */
          *r_key = xtrymalloc (datalen);
          if (!*r_key)
            err = gpg_err_make (default_errsource,
                                gpg_err_code_from_syserror ());
          else
            {
              memcpy (*r_key, data, datalen);
              *r_keylen = datalen;
              err = 0;
            }
          goto leave;
        }

      if (datalen < 5)
        continue;  /* Truncated CERT record - skip.  */

      ctype = buf16_to_uint (data);
      /* (key tag and algorithm fields are not required.) */
      data += 5;
      datalen -= 5;

      if (want_certtype && want_certtype != ctype)
        ; /* Not of the requested certtype.  */
      else if (ctype == DNS_CERTTYPE_PGP && datalen >= 11 && r_key && r_keylen)
        {
          /* CERT type is PGP.  Gpg checks for a minimum length of 11,
             thus we do the same.  */
          *r_key = xtrymalloc (datalen);
          if (!*r_key)
            err = gpg_err_make (default_errsource,
                                gpg_err_code_from_syserror ());
          else
            {
              memcpy (*r_key, data, datalen);
              *r_keylen = datalen;
              err = 0;
            }
          goto leave;
        }
      else if (ctype == DNS_CERTTYPE_IPGP && datalen && datalen < 1023
               && datalen >= data[0] + 1 && r_fpr && r_fprlen && r_url)
        {
          /* CERT type is IPGP.  We made sure that the data is
             plausible and that the caller requested this
             information.  */
          *r_fprlen = data[0];
          if (*r_fprlen)
            {
              *r_fpr = xtrymalloc (*r_fprlen);
              if (!*r_fpr)
                {
                  err = gpg_err_make (default_errsource,
                                      gpg_err_code_from_syserror ());
                  goto leave;
                }
              memcpy (*r_fpr, data + 1, *r_fprlen);
            }
          else
            *r_fpr = NULL;

          if (datalen > *r_fprlen + 1)
            {
              *r_url = xtrymalloc (datalen - (*r_fprlen + 1) + 1);
              if (!*r_url)
                {
                  err = gpg_err_make (default_errsource,
                                      gpg_err_code_from_syserror ());
                  xfree (*r_fpr);
                  *r_fpr = NULL;
                  goto leave;
                }
              memcpy (*r_url,
                      data + (*r_fprlen + 1), datalen - (*r_fprlen + 1));
              (*r_url)[datalen - (*r_fprlen + 1)] = '\0';
            }
          else
            *r_url = NULL;

          err = 0;
          goto leave;
        }
    }

 leave:
  adns_free (answer);
  adns_finish (state);
  return err;

#else /*!USE_ADNS*/

  gpg_error_t err;
  unsigned char *answer;
  int r;
  u16 count;

  if (r_key)
    *r_key = NULL;
  if (r_keylen)
    *r_keylen = 0;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  /* Allocate a 64k buffer which is the limit for an DNS response.  */
  answer = xtrymalloc (65536);
  if (!answer)
    return gpg_err_make (default_errsource, gpg_err_code_from_syserror ());

  err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);

  r = res_query (name, C_IN,
                 (want_certtype < DNS_CERTTYPE_RRBASE
                  ? T_CERT
                  : (want_certtype - DNS_CERTTYPE_RRBASE)),
                 answer, 65536);
  /* Not too big, not too small, no errors and at least 1 answer. */
  if (r >= sizeof (HEADER) && r <= 65536
      && (((HEADER *) answer)->rcode) == NOERROR
      && (count = ntohs (((HEADER *) answer)->ancount)))
    {
      int rc;
      unsigned char *pt, *emsg;

      emsg = &answer[r];

      pt = &answer[sizeof (HEADER)];

      /* Skip over the query */

      rc = dn_skipname (pt, emsg);
      if (rc == -1)
        {
          err = gpg_err_make (default_errsource, GPG_ERR_INV_OBJ);
          goto leave;
        }
      pt += rc + QFIXEDSZ;

      /* There are several possible response types for a CERT request.
         We're interested in the PGP (a key) and IPGP (a URI) types.
         Skip all others.  TODO: A key is better than a URI since
         we've gone through all this bother to fetch it, so favor that
         if we have both PGP and IPGP? */

      while (count-- > 0 && pt < emsg)
        {
          u16 type, class, dlen, ctype;

          rc = dn_skipname (pt, emsg);  /* the name we just queried for */
          if (rc == -1)
            {
              err = gpg_err_make (default_errsource, GPG_ERR_INV_OBJ);
              goto leave;
            }

          pt += rc;

          /* Truncated message? 15 bytes takes us to the point where
             we start looking at the ctype. */
          if ((emsg - pt) < 15)
            break;

          type = buf16_to_u16 (pt);
          pt += 2;

          class = buf16_to_u16 (pt);
          pt += 2;

          if (class != C_IN)
            break;

          /* ttl */
          pt += 4;

          /* data length */
          dlen = buf16_to_u16 (pt);
          pt += 2;

          /* Check the type and parse.  */
          if (want_certtype >= DNS_CERTTYPE_RRBASE
              && type == (want_certtype - DNS_CERTTYPE_RRBASE)
              && r_key)
            {
              *r_key = xtrymalloc (dlen);
              if (!*r_key)
                err = gpg_err_make (default_errsource,
                                    gpg_err_code_from_syserror ());
              else
                {
                  memcpy (*r_key, pt, dlen);
                  *r_keylen = dlen;
                  err = 0;
                }
              goto leave;
            }
          else if (want_certtype >= DNS_CERTTYPE_RRBASE)
            {
              /* We did not found the requested RR.  */
              pt += dlen;
            }
          else if (type == T_CERT)
            {
              /* We got a CERT type.   */
              ctype = buf16_to_u16 (pt);
              pt += 2;

              /* Skip the CERT key tag and algo which we don't need. */
              pt += 3;

              dlen -= 5;

              /* 15 bytes takes us to here */
              if (want_certtype && want_certtype != ctype)
                ; /* Not of the requested certtype.  */
              else if (ctype == DNS_CERTTYPE_PGP && dlen && r_key && r_keylen)
                {
                  /* PGP type */
                  *r_key = xtrymalloc (dlen);
                  if (!*r_key)
                    err = gpg_err_make (default_errsource,
                                        gpg_err_code_from_syserror ());
                  else
                    {
                      memcpy (*r_key, pt, dlen);
                      *r_keylen = dlen;
                      err = 0;
                    }
                  goto leave;
                }
              else if (ctype == DNS_CERTTYPE_IPGP
                       && dlen && dlen < 1023 && dlen >= pt[0] + 1)
                {
                  /* IPGP type */
                  *r_fprlen = pt[0];
                  if (*r_fprlen)
                    {
                      *r_fpr = xtrymalloc (*r_fprlen);
                      if (!*r_fpr)
                        {
                          err = gpg_err_make (default_errsource,
                                              gpg_err_code_from_syserror ());
                          goto leave;
                        }
                      memcpy (*r_fpr, &pt[1], *r_fprlen);
                    }
                  else
                    *r_fpr = NULL;

                  if (dlen > *r_fprlen + 1)
                    {
                      *r_url = xtrymalloc (dlen - (*r_fprlen + 1) + 1);
                      if (!*r_fpr)
                        {
                          err = gpg_err_make (default_errsource,
                                              gpg_err_code_from_syserror ());
                          xfree (*r_fpr);
                          *r_fpr = NULL;
                          goto leave;
                        }
                      memcpy (*r_url, &pt[*r_fprlen + 1],
                              dlen - (*r_fprlen + 1));
                      (*r_url)[dlen - (*r_fprlen + 1)] = '\0';
                    }
                  else
                    *r_url = NULL;

                  err = 0;
                  goto leave;
                }

              /* No subtype matches, so continue with the next answer. */
              pt += dlen;
            }
          else
            {
              /* Not a requested type - might be a CNAME. Try next item.  */
              pt += dlen;
            }
        }
    }

 leave:
  xfree (answer);
  return err;

#endif /*!USE_ADNS */
#else /* !USE_DNS_CERT */
  (void)name;
  if (r_key)
    *r_key = NULL;
  if (r_keylen)
    *r_keylen = NULL;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  return gpg_err_make (default_errsource, GPG_ERR_NOT_SUPPORTED);
#endif
}
