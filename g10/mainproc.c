/* mainproc.c - handle packets
 *	Copyright (c) 1997 by Werner Koch (dd9jn)
 *
 * This file is part of G10.
 *
 * G10 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * G10 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "packet.h"
#include "iobuf.h"
#include "memory.h"
#include "options.h"
#include "util.h"
#include "cipher.h"
#include "keydb.h"
#include "filter.h"
#include "cipher.h"
#include "main.h"


/****************
 * We need to glue the packets together.  This done by a
 * tree of packets, which will released whenever a new start packet
 * is encounterd. Start packets are: [FIXME]
 *
 *  pubkey
 *     userid		   userid
 *	  sig, sig, sig       sig, sig
 *
 */

typedef struct node_struct *NODE;
struct node_struct {
    PACKET *pkt;
    NODE next;	 /* used to form a link list */
    NODE child;
};


/****************
 * Structure to hold the context
 */

typedef struct {
    PKT_public_cert *last_pubkey;
    PKT_secret_cert *last_seckey;
    PKT_user_id     *last_user_id;
    md_filter_context_t mfx;
    DEK *dek;
    int last_was_pubkey_enc;
    int opt_list;
    NODE cert;	   /* the current certificate */
    int have_data;
} *CTX;






static void list_node( CTX c, NODE node );
static void proc_tree( CTX c, NODE node );

static int
pubkey_letter( int algo )
{
    switch( algo ) {
      case PUBKEY_ALGO_RSA:	return 'R' ;
      case PUBKEY_ALGO_RSA_E:	return 'r' ;
      case PUBKEY_ALGO_RSA_S:	return 's' ;
      case PUBKEY_ALGO_ELGAMAL: return 'G' ;
      case PUBKEY_ALGO_DSA:	return 'D' ;
      default: return '?';
    }
}



static NODE
new_node( PACKET *pkt )
{
    NODE n = m_alloc( sizeof *n );
    n->next = NULL;
    n->pkt = pkt;
    n->child = NULL;
    return n;
}


static void
release_node( NODE n )
{
    NODE n2;

    while( n ) {
	n2 = n->next;
	release_node( n->child );
	free_packet( n->pkt );
	m_free( n );
	n = n2;
    }
}


/****************
 * Return the parent node of NODE from the tree with ROOT
 */
static NODE
find_parent( NODE root, NODE node )
{
    NODE n, n2;

    for( ; root; root = root->child) {
	for( n = root; n; n = n->next) {
	    for( n2 = n->child; n2; n2 = n2->next ) {
		if( n2 == node )
		    return n;
	    }
	}
    }
    log_bug(NULL);
}


static void
release_cert( CTX c )
{
    if( !c->cert )
	return;
    proc_tree(c, c->cert );
    release_node( c->cert );
    c->cert = NULL;
}


static int
add_onepass_sig( CTX c, PACKET *pkt )
{
    NODE node;

    if( c->cert ) { /* add another packet */

	if( c->cert->pkt->pkttype != PKT_ONEPASS_SIG ) {
	   log_error("add_onepass_sig: another packet is in the way\n");
	   release_cert( c );
	}
	node = new_node( pkt );
	node->next = c->cert;
	c->cert = node;
    }
    else /* insert the first one */
	c->cert = node = new_node( pkt );

    return 1;
}


static int
add_public_cert( CTX c, PACKET *pkt )
{
    release_cert( c );
    c->cert = new_node( pkt );
    return 1;
}

static int
add_secret_cert( CTX c, PACKET *pkt )
{
    release_cert( c );
    c->cert = new_node( pkt );
    return 1;
}


static int
add_user_id( CTX c, PACKET *pkt )
{
    u32 keyid[2];
    NODE node, n1, n2;

    if( !c->cert ) {
	log_error("orphaned user id\n" );
	return 0;
    }
    /* goto the last certificate (currently ther is only one) */
    for(n1=c->cert; n1->next; n1 = n1->next )
	;
    assert( n1->pkt );
    if( n1->pkt->pkttype != PKT_PUBLIC_CERT
	&& n1->pkt->pkttype != PKT_SECRET_CERT ) {
	log_error("invalid parent type %d for userid\n", n1->pkt->pkttype );
	return 0;
    }
    /* add a new user id node at the end */
    node = new_node( pkt );
    if( !(n2=n1->child) )
	n1->child = node;
    else {
	for( ; n2->next; n2 = n2->next)
	    ;
	n2->next = node;
    }
    return 1;
}


static int
add_signature( CTX c, PACKET *pkt )
{
    u32 keyid[2];
    NODE node, n1, n2;

    if( !c->cert ) {
	/* orphaned signature (no certificate)
	 * this is the first signature for a following datafile
	 */
	return 0;
    }
    assert( c->cert->pkt );
    if( c->cert->pkt->pkttype == PKT_ONEPASS_SIG ) {
	/* The root is a onepass signature, so we are signing data
	 * The childs direct under the root are the signatures
	 * (there is no need to keep the correct sequence of packets)
	 */
	node = new_node( pkt );
	node->next = c->cert->child;
	c->cert->child = node;
	return 1;
    }


    if( !c->cert->child ) {
	log_error("orphaned signature (no userid)\n" );
	return 0;
    }
    /* goto the last user id */
    for(n1=c->cert->child; n1->next; n1 = n1->next )
	;
    assert( n1->pkt );
    if( n1->pkt->pkttype != PKT_USER_ID ) {
	log_error("invalid parent type %d for sig\n", n1->pkt->pkttype);
	return 0;
    }
    /* and add a new signature node id at the end */
    node = new_node( pkt );
    if( !(n2=n1->child) )
	n1->child = node;
    else {
	for( ; n2->next; n2 = n2->next)
	    ;
	n2->next = node;
    }
    return 1;
}


static void
proc_pubkey_enc( CTX c, PACKET *pkt )
{
    PKT_pubkey_enc *enc;
    int result = 0;

    c->last_was_pubkey_enc = 1;
    enc = pkt->pkt.pubkey_enc;
    printf("enc: encrypted by a pubkey with keyid %08lX\n", enc->keyid[1] );
    if( enc->pubkey_algo == PUBKEY_ALGO_ELGAMAL
	|| enc->pubkey_algo == PUBKEY_ALGO_RSA	) {
	m_free(c->dek ); /* paranoid: delete a pending DEK */
	c->dek = m_alloc_secure( sizeof *c->dek );
	if( (result = get_session_key( enc, c->dek )) ) {
	    /* error: delete the DEK */
	    m_free(c->dek); c->dek = NULL;
	}
    }
    else
	result = G10ERR_PUBKEY_ALGO;

    if( result == -1 )
	;
    else if( !result )
	fputs(	"     DEK is good", stdout );
    else
	printf( "     %s", g10_errstr(result));
    putchar('\n');
    free_packet(pkt);
}



static void
proc_encrypted( CTX c, PACKET *pkt )
{
    int result = 0;

    printf("dat: %sencrypted data\n", c->dek?"":"conventional ");
    if( !c->dek && !c->last_was_pubkey_enc ) {
	/* assume this is conventional encrypted data */
	c->dek = m_alloc_secure( sizeof *c->dek );
	c->dek->algo = DEFAULT_CIPHER_ALGO;
	result = make_dek_from_passphrase( c->dek, 0 );
    }
    else if( !c->dek )
	result = G10ERR_NO_SECKEY;
    if( !result )
	result = decrypt_data( pkt->pkt.encrypted, c->dek );
    m_free(c->dek); c->dek = NULL;
    if( result == -1 )
	;
    else if( !result )
	fputs(	"     encryption okay",stdout);
    else
	printf( "     %s", g10_errstr(result));
    putchar('\n');
    free_packet(pkt);
    c->last_was_pubkey_enc = 0;
}


static void
proc_plaintext( CTX c, PACKET *pkt )
{
    PKT_plaintext *pt = pkt->pkt.plaintext;
    int result;

    printf("txt: plain text data name='%.*s'\n", pt->namelen, pt->name);
    free_md_filter_context( &c->mfx );
    /* fixme: take the digest algo to use from the
     * onpass_sig packet (if we have these) */
    c->mfx.md = md_open(DIGEST_ALGO_RMD160, 0);
    result = handle_plaintext( pt, &c->mfx );
    if( !result )
	fputs(	"     okay", stdout);
    else
	printf( "     %s", g10_errstr(result));
    putchar('\n');
    free_packet(pkt);
    c->last_was_pubkey_enc = 0;
}


static void
proc_compressed( CTX c, PACKET *pkt )
{
    PKT_compressed *zd = pkt->pkt.compressed;
    int result;

    printf("zip: compressed data packet\n");
    result = handle_compressed( zd );
    if( !result )
	fputs(	"     okay", stdout);
    else
	printf( "     %s", g10_errstr(result));
    putchar('\n');
    free_packet(pkt);
    c->last_was_pubkey_enc = 0;
}




/****************
 * check the signature
 * Returns: 0 = valid signature or an error code
 */
static int
do_check_sig( CTX c, NODE node )
{
    PKT_signature *sig;
    MD_HANDLE *md;
    int algo, rc;

    assert( node->pkt->pkttype == PKT_SIGNATURE );
    sig = node->pkt->pkt.signature;

    if( sig->pubkey_algo == PUBKEY_ALGO_ELGAMAL )
	algo = sig->d.elg.digest_algo;
    else if(sig->pubkey_algo == PUBKEY_ALGO_RSA )
	algo = sig->d.rsa.digest_algo;
    else
	return G10ERR_PUBKEY_ALGO;
    if( (rc=md_okay(algo)) )
	return rc;

    if( sig->sig_class == 0x00 ) {
	md = md_copy( c->mfx.md );
    }
    else if( (sig->sig_class&~3) == 0x10 ) { /* classes 0x10 .. 0x13 */
	if( c->cert->pkt->pkttype == PKT_PUBLIC_CERT ) {
	    NODE n1 = find_parent( c->cert, node );

	    if( n1 && n1->pkt->pkttype == PKT_USER_ID ) {

		if( c->cert->pkt->pkt.public_cert->mfx.md )
		    md = md_copy( c->cert->pkt->pkt.public_cert->mfx.md );
		else if( algo == DIGEST_ALGO_RMD160 )
		    md = rmd160_copy2md( c->cert->pkt->pkt.public_cert->mfx.rmd160 );
		else if( algo == DIGEST_ALGO_MD5 )
		    md = md5_copy2md( c->cert->pkt->pkt.public_cert->mfx.md5 );
		else
		    log_bug(NULL);
		md_write( md, n1->pkt->pkt.user_id->name, n1->pkt->pkt.user_id->len);
	    }
	    else {
		log_error("invalid parent packet for sigclass 0x10\n");
		return G10ERR_SIG_CLASS;
	    }
	}
	else {
	    log_error("invalid root packet for sigclass 0x10\n");
	    return G10ERR_SIG_CLASS;
	}
    }
    else
	return G10ERR_SIG_CLASS;
    rc = signature_check( sig, md );
    md_close(md);

    return rc;
}



static void
print_userid( PACKET *pkt )
{
    if( !pkt )
	log_bug(NULL);
    if( pkt->pkttype != PKT_USER_ID ) {
	printf("ERROR: unexpected packet type %d", pkt->pkttype );
	return;
    }
    print_string( stdout,  pkt->pkt.user_id->name, pkt->pkt.user_id->len );
}


/****************
 * List the certificate in a user friendly way
 */

static void
list_node( CTX c, NODE node )
{
    register NODE n2;

    if( !node )
	;
    else if( node->pkt->pkttype == PKT_PUBLIC_CERT ) {
	PKT_public_cert *pkc = node->pkt->pkt.public_cert;

	printf("pub  %4u%c/%08lX %s ", nbits_from_pkc( pkc ),
				      pubkey_letter( pkc->pubkey_algo ),
				      (ulong)keyid_from_pkc( pkc, NULL ),
				      datestr_from_pkc( pkc )	  );
	n2 = node->child;
	if( !n2 )
	    printf("ERROR: no user id!\n");
	else {
	    /* and now list all userids with their signatures */
	    for( ; n2; n2 = n2->next ) {
		if( n2 != node->child )
		    printf( "%*s", 31, "" );
		print_userid( n2->pkt );
		putchar('\n');
		list_node(c,  n2 );
	    }
	}
    }
    else if( node->pkt->pkttype == PKT_SECRET_CERT ) {
	PKT_secret_cert *skc = node->pkt->pkt.secret_cert;

	printf("sec  %4u%c/%08lX %s ", nbits_from_skc( skc ),
				      pubkey_letter( skc->pubkey_algo ),
				      (ulong)keyid_from_skc( skc, NULL ),
				      datestr_from_skc( skc )	  );
	n2 = node->child;
	if( !n2 )
	    printf("ERROR: no user id!");
	else {
	    print_userid( n2->pkt );
	}
	putchar('\n');
    }
    else if( node->pkt->pkttype == PKT_USER_ID ) {
	/* list everything under this user id */
	for(n2=node->child; n2; n2 = n2->next )
	    list_node(c,  n2 );
    }
    else if( node->pkt->pkttype == PKT_SIGNATURE ) {
	PKT_signature *sig = node->pkt->pkt.signature;
	int rc2;
	size_t n;
	char *p;
	int sigrc = ' ';

	assert( !node->child );
	if( opt.check_sigs ) {

	    switch( (rc2=do_check_sig( c, node )) ) {
	      case 0:		       sigrc = '!'; break;
	      case G10ERR_BAD_SIGN:    sigrc = '-'; break;
	      case G10ERR_NO_PUBKEY:   sigrc = '?'; break;
	      default:		       sigrc = '%'; break;
	    }
	}
	printf("sig%c       %08lX %s   ",
		sigrc, sig->keyid[1], datestr_from_sig(sig));
	if( sigrc == '%' )
	    printf("[%s] ", g10_errstr(rc2) );
	else if( sigrc == '?' )
	    ;
	else {
	    p = get_user_id( sig->keyid, &n );
	    print_string( stdout, p, n );
	    m_free(p);
	}
	putchar('\n');
    }
    else
	log_error("invalid node with packet of type %d\n", node->pkt->pkttype);
}


int
proc_packets( IOBUF a )
{
    CTX c = m_alloc_clear( sizeof *c );
    PACKET *pkt = m_alloc( sizeof *pkt );
    int rc, result;
    char *ustr;
    int lvl0, lvl1;
    u32 keyid[2];
    int newpkt;

    c->opt_list = 1;
    init_packet(pkt);
    while( (rc=parse_packet(a, pkt)) != -1 ) {
	/* cleanup if we have an illegal data structure */
	if( c->dek && pkt->pkttype != PKT_ENCRYPTED ) {
	    log_error("oops: valid pubkey enc packet not followed by data\n");
	    m_free(c->dek); c->dek = NULL; /* burn it */
	}

	if( rc ) {
	    free_packet(pkt);
	    continue;
	}
	newpkt = -1;
	switch( pkt->pkttype ) {
	  case PKT_PUBLIC_CERT: newpkt = add_public_cert( c, pkt ); break;
	  case PKT_SECRET_CERT: newpkt = add_secret_cert( c, pkt ); break;
	  case PKT_USER_ID:	newpkt = add_user_id( c, pkt ); break;
	  case PKT_SIGNATURE:	newpkt = add_signature( c, pkt ); break;
	  case PKT_PUBKEY_ENC:	proc_pubkey_enc( c, pkt ); break;
	  case PKT_ENCRYPTED:	proc_encrypted( c, pkt ); break;
	  case PKT_PLAINTEXT:	proc_plaintext( c, pkt ); break;
	  case PKT_COMPRESSED:	proc_compressed( c, pkt ); break;
	  case PKT_ONEPASS_SIG: newpkt = add_onepass_sig( c, pkt ); break;
	  default: newpkt = 0; break;
	}
	if( pkt->pkttype != PKT_SIGNATURE )
	    c->have_data = pkt->pkttype == PKT_PLAINTEXT;

	if( newpkt == -1 )
	    ;
	else if( newpkt ) {
	    pkt = m_alloc( sizeof *pkt );
	    init_packet(pkt);
	}
	else
	    free_packet(pkt);
    }

    release_cert( c );
    m_free(c->dek);
    free_packet( pkt );
    m_free( pkt );
    free_md_filter_context( &c->mfx );
    m_free( c );
    return 0;
}


static void
print_keyid( FILE *fp, u32 *keyid )
{
    size_t n;
    char *p = get_user_id( keyid, &n );
    print_string( fp, p, n );
    m_free(p);
}

/****************
 * Preocess the tree which starts at node
 */
static void
proc_tree( CTX c, NODE node )
{
    NODE n1;
    int rc;

    if( node->pkt->pkttype == PKT_PUBLIC_CERT )
	list_node( c, node );
    else if( node->pkt->pkttype == PKT_SECRET_CERT )
	list_node( c, node );
    else if( node->pkt->pkttype == PKT_ONEPASS_SIG ) {
	if( !node->child )
	    log_error("proc_tree: onepass_sig without followin data\n");
	else if( node->child->pkt->pkttype != PKT_SIGNATURE )
	    log_error("proc_tree: onepass_sig not followed by signature\n");
	else {	/* check all signatures */
	    if( !c->have_data ) {
		free_md_filter_context( &c->mfx );
		/* fixme: take the digest algo to use from the
		 * onepass_sig packet (if we have these) */
		c->mfx.md = md_open(DIGEST_ALGO_RMD160, 0);
		rc = ask_for_detached_datafile( &c->mfx );
		if( rc ) {
		    log_error("can't hash datafile: %s\n", g10_errstr(rc));
		    return;
		}
	    }

	    for(n1=node->child; n1; n1 = n1->next ) {
		PKT_signature *sig = n1->pkt->pkt.signature;

		rc = do_check_sig(c, n1 );
		if( !rc ) {
		    log_info("Good signature from ");
		    print_keyid( stderr, sig->keyid );
		    putc('\n', stderr);
		}
		else if( rc == G10ERR_BAD_SIGN ) {
		    log_error("BAD signature from ");
		    print_keyid( stderr, sig->keyid );
		    putc('\n', stderr);
		}
		else
		    log_error("Can't check signature made by %08lX: %s\n",
			       sig->keyid[1], g10_errstr(rc) );
	    }
	}
    }
    else if( node->pkt->pkttype == PKT_SIGNATURE ) {
	log_info("proc_tree: old style signature\n");
    }
    else
	log_error("proc_tree: invalid root packet\n");

}



