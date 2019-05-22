/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * (c) Copyright 2012 Samsung Telecommunication America, Inc.
 *                  All rights reserved
 *
 *             Wireless Terminals Lab in Dallas Technology Lab 
 *
 *
 * File: SSLEngine.h
 * Author: xzhou@sta.samsung.com
 * Creation date: Thu Oct 25 12:00:00 2012
 * Rev: $Id$
 *
 */

#ifndef _SEC_SSL_ENGINE_H
#define _SEC_SSL_ENGINE_H

#include <openssl/engine.h>
#define ENG_OK                         0
/*
* Error codes for registerEngine
*/
#define ENG_GET_PIN_FAILED             1
#define ENG_SETUP_FAILED               2
#define ENG_GET_CERT_FAILED            3

/*
*Error codes for setupCredentials
*/
#define ENG_GET_PRIV_KEY_FAILED        10
#define ENG_LOAD_CERT_FAILED           11



#ifdef __cplusplus
extern "C" {
#endif
	/**
     * API to register a smartcard OpenSSL engine.
     * 
     * @usage Application uses this API to register with the OpenSSL engine provided by a smartcard.
	 * it calls SmartCardHelper java class to get the pin.
     * <p>
     * Example call:<p>
     * <pre>
     *  
     *  int ret = registerEngine();
	 *  if(ret != 0 )  { // failure                  
	 *     // failure code
     *  } else {
	 *     // success code
     *  }
     * </pre>
     *            
     * @return  non zero error code on failure and 0 on success
     * 
     *
     */
	int registerEngine();

	/**
     * API to create a SSL_CTX object.
     * 
     * @usage Application uses this API to create a SSL_CTX object from OpenSSL engine provided by a smartcard.	 
     * <p>
     * Example call:<p>
     * <pre>
     *  
     *  SSL_CTX* ctx;
	 *  int ret = setupCredentials(&ctx);
	 *  if(ret != 0)  { // failure                  
	 *     // failure code
     *  } else {
	 *     // success code
     *  }
     * </pre>
     *            
     * @return  non-zero error code on failure and 0 on success
     * 
     *
     */
	int setupCredentials(SSL_CTX**);

	/**
     * API to unregister the Engine.
     * 
     * @usage Application uses this API to unregister a OpenSSL engine provided by a smartcard.	 
     * <p>
     * Example call:<p>
     * <pre>
     *  
	 *  int ret = deregisterEngine();
	 *  if(ret != 0)  { // failure                  
	 *     // failure code
     *  } else {
	 *     // success code
     *  }
     * </pre>
     *            
     * @return  non zero error code on failure and 0 on success
     * 
     *
     */
	 int deregisterEngine();

	 /**
	  * API to retrieve identity public certificate from the card.
	  * This should be called before getPrivKeyFromCard() function call.
	  *
	  * @return  new instance of X509 certificate
	  *
	  *
	  */
    X509* getCertFromCard ();

	 /**
	  * API to retrieve private key handle from the card.
	  * This API will register pkcs11 ssl engine, load ssl engine and load the private key
	  *
	  * @return  new instance of private key
	  *
	  *
	  */
    EVP_PKEY* getPrivKeyFromCard();

	 /**
	  * API to register a smartcard OpenSSL engine.
	  *
	  * @return  the registered SSL engine.
	  *
	  *
	  */
    ENGINE* registerSSLEngine();




#ifdef __cplusplus
};
#endif


#endif