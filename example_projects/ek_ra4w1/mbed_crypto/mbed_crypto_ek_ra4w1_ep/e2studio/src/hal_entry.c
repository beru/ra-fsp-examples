/***********************************************************************************************************************
 * File Name    : hal_entry.c
 * Description  : Contains data structures and functions used in hal_entry.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2020 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/

#include "common_utils.h"
#include "crypto_ep.h"

/*******************************************************************************************************************//**
 * @addtogroup crypto_ep
 * @{
 **********************************************************************************************************************/

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

/* Global variables.*/
/* Nonce or IV to use.*/
static const uint8_t nonce[12] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                   0x08, 0x09, 0x0A, 0x0B };
/* Additional data that will be authenticated but not encrypted.*/
static const uint8_t additional_data[12] = {RESET_VALUE};
/* Data that will be authenticated and encrypted.*/
static const uint8_t input_data[] = { 0xB9, 0x6B, 0x49, 0xE2, 0x1D, 0x62, 0x17, 0x41,
                                      0x63, 0x28, 0x75, 0xDB, 0x7F, 0x6C, 0x92, 0x43,
                                      0xD2, 0xD7, 0xC2 };
/* platform context structure.*/
mbedtls_platform_context ctx = {RESET_VALUE};

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    fsp_pack_version_t version                   = {RESET_VALUE};
    psa_status_t       status                    = (psa_status_t)RESET_VALUE;
    int                mbed_ret_val              = RESET_VALUE;

    /* version get API for FLEX pack information */
    R_FSP_VersionGet(&version);

    /* Example Project information printed on the RTT */
    APP_PRINT(BANNER_INFO, EP_VERSION, version.version_id_b.major, version.version_id_b.minor, version.version_id_b.patch);
    APP_PRINT(EP_INFO);

    /* Setup the platform; initialize the SCE */
    mbed_ret_val = mbedtls_platform_setup(&ctx);
    if (RESET_VALUE != mbed_ret_val)
    {
        APP_ERR_PRINT("\r\n** mbedtls_platform_setup API FAILED ** \r\n");
        APP_ERR_TRAP(mbed_ret_val);
    }

    /* Initialize crypto library.*/
    status = psa_crypto_init();
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_crypto_init API FAILED ** \r\n");
        /* De-initialize the platform.*/
        mbedtls_platform_teardown(&ctx);
        APP_ERR_TRAP(status);
    }

    /* Perform AES operation.*/
    status = aes_operation();
    if (PSA_SUCCESS != status)
    {
        /*AES operation failed. Perform cleanup and trap error. */
        handle_error(status, "\r\n** AES OPERATION FAILED ** \r\n");
    }

    /* RA4M1 and RA4W1 boards are not supporting SHA, ECC and RSA operations.*/
#if (! (defined (BOARD_RA4M1_EK) || defined (BOARD_RA4W1_EK) || defined (BOARD_RA2L1_EK) || defined (BOARD_RA2A1_EK)))
    /* Perform Hashing operation.*/
    status = sha_operation();
    if (PSA_SUCCESS != status)
    {
        /*SHA operation failed. Perform cleanup and trap error. */
        handle_error(status, "\r\n** HASHING OPERATION FAILED ** \r\n");
    }

    /* Perform ECC operation.*/
    status = ecc_operation();
    if (PSA_SUCCESS != status)
    {
        /*ECC operation failed. Perform cleanup and trap error. */
        handle_error(status, "\r\n** ECC OPERATION FAILED ** \r\n");
    }

    /* Perform RSA operation.*/
    status = rsa_operation();
    if (PSA_SUCCESS != status)
    {
        /*RSA operation failed. Perform cleanup and trap error. */
        handle_error(status, "\r\n** RSA OPERATION FAILED ** \r\n");
    }
#endif
    APP_PRINT("\r\nCrypto Operations completed.\r\n");

    /* De-initialize the platform.*/
    mbedtls_psa_crypto_free();
    mbedtls_platform_teardown(&ctx);
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event) {
	if (BSP_WARM_START_RESET == event) {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
	}

	if (BSP_WARM_START_POST_C == event) {
		/* C runtime environment and system clocks are setup. */

		/* Configure pins. */
		R_IOPORT_Open(&g_ioport_ctrl, &g_bsp_pin_cfg);
	}
}

/*******************************************************************************************************************//**
 *  @brief       Performs AES256 Key generation(for GCM Mode), Encryption and Decryption.
 *  @param[IN]   None
 *  @retval      PSA_SUCCESS or any other possible error code
 **********************************************************************************************************************/
psa_status_t aes_operation(void)
{
    psa_status_t           status          = (psa_status_t)RESET_VALUE;
    psa_key_attributes_t   attributes      = PSA_KEY_ATTRIBUTES_INIT; //Contains key attributes.
    const psa_algorithm_t  alg             = PSA_ALG_GCM;     // GCM authenticated encryption algorithm.
    psa_key_handle_t       aes_key_handle  = RESET_VALUE;     // AES Key handle.
    uint8_t                *output_data    = NULL;            // Output buffer for the authenticated and encrypted data.
    uint8_t                *output_data1   = NULL;            // Output buffer for the decrypted data.
    size_t                 output_size     = RESET_VALUE;     // Size of the output buffer in bytes.
    size_t                 output_length   = RESET_VALUE;     // the size of the output in the encrypted output buffer.
    size_t                 output_length1  = RESET_VALUE;     // the size of the output in the decrypted output buffer.

    /* RA4M1 and RA4W1 boards are not supporting Littlefs, so using key lifetime as volatile.*/
#if (! (defined (BOARD_RA4M1_EK) || defined (BOARD_RA4W1_EK) || defined (BOARD_RA2L1_EK) || defined (BOARD_RA2A1_EK)))
    uint8_t              aes_key[AES_256_EXPORTED_SIZE] = {RESET_VALUE};  // Buffer where the key data is to be written.
    size_t               aes_key_length                 = RESET_VALUE;    // number of bytes that make up the key data.
    fsp_err_t            err                            = FSP_SUCCESS;
    psa_key_lifetime_t   lifetime = PSA_KEY_LIFETIME_PERSISTENT;
    psa_key_usage_t      key_flag = (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_EXPORT);
    /* Perform littlefs operation.*/
    err = littlefs_init();
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** littlefs operation failed. ** \r\n");
        return err;
    }
#else
    psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_VOLATILE;
    psa_key_usage_t    key_flag = (PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
#endif

    /* Set Key uses flags, key_algorithm, key_type, key_bits, key_lifetime, key_id*/
    psa_set_key_usage_flags(&attributes, key_flag);
    psa_set_key_algorithm(&attributes, alg);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, AES_KEY_BITS);
    psa_set_key_lifetime(&attributes, lifetime);

    if (PSA_KEY_LIFETIME_IS_PERSISTENT(lifetime))
    {
        /* Set the id to a positive integer. */
        psa_set_key_id(&attributes, AES_KEY_ID);
    }

    APP_PRINT("\r\nAES Operation Started.\r\n");

    /* Generating AES 256 key and allocating to key slot.*/
    status = psa_generate_key(&attributes, &aes_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_generate_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("AES Key generated Successfully.\r\n");

    /* Calculate output size for encryption and decryption.*/
    output_size = sizeof(input_data) + TAG_LENGTH;
    output_data = (uint8_t *)malloc(output_size);
    output_data1 = (uint8_t *)malloc(output_size);

    if((NULL == output_data) || (NULL == output_data1))
    {
        APP_ERR_PRINT("\r\n** Out Of Memory. ** \r\n");
        return FSP_ERR_OUT_OF_MEMORY;
    }
    /* Authenticate and encrypt */
    status = psa_aead_encrypt(aes_key_handle, alg,
                              nonce, sizeof(nonce),
                              additional_data, sizeof(additional_data),
                              input_data, sizeof(input_data),
                              output_data, output_size,
                              &output_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_aead_encrypt API FAILED ** \r\n");
        return status;
    }

#if (! (defined (BOARD_RA4M1_EK) || defined (BOARD_RA4W1_EK) || defined (BOARD_RA2L1_EK) || defined (BOARD_RA2A1_EK)))
    /* Export Key.*/
    status = psa_export_key(aes_key_handle, aes_key, sizeof(aes_key), &aes_key_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_export_key API FAILED ** \r\n");
        return status;
    }

    /* Destroy Key.*/
    status = psa_destroy_key(aes_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_destroy_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("Exported and Destroyed Key Successfully.\r\n");

    /* This is intended to fail as key was not imported after destroying.*/
    /* Authenticate and decrypt */
    status = psa_aead_decrypt(aes_key_handle, alg,
                              nonce, sizeof(nonce),
                              additional_data, sizeof(additional_data),
                              output_data, output_length,
                              output_data1, output_length,
                              &output_length1);
    if (PSA_SUCCESS == status)
    {
        APP_ERR_PRINT("\r\n** psa_aead_decrypt API Should Fail.** \r\n");
        return status;
    }
    APP_PRINT("aead decryption failed as key was destroyed.\r\n");

    /* Import key.*/
    status = psa_import_key(&attributes, aes_key, aes_key_length, &aes_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_import_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("Imported Key Successfully.\r\n");
#endif

    /* Authenticate and decrypt */
    status = psa_aead_decrypt(aes_key_handle, alg,
                              nonce, sizeof(nonce),
                              additional_data, sizeof(additional_data),
                              output_data, output_length,
                              output_data1, output_length,
                              &output_length1);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_aead_decrypt API FAILED ** \r\n");
        return status;
    }

    /* Compare the encrypted data length and decrypted data length.*/
    if(output_length1 != sizeof(input_data))
    {
        APP_ERR_PRINT("\r\n** Comparison of encrypted data length and decrypted data length failed.** \r\n");
        return FSP_ERR_CRYPTO_INVALID_SIZE;
    }

    /* Compare the encrypted data and decrypted data.*/
    for(int len = RESET_VALUE; len < (int )output_length1; len++)
    {
        if(input_data[len] != *output_data1)
        {
            APP_ERR_PRINT("\r\n** Comparison of encrypted data and decrypted data failed.** \r\n");
            return FSP_ERR_ABORTED;
        }
        output_data1++;
    }

    APP_PRINT("aead encryption and decryption completed successfully.\r\n");
    return status;
}

#if (! (defined (BOARD_RA4M1_EK) || defined (BOARD_RA4W1_EK) || defined (BOARD_RA2L1_EK) || defined (BOARD_RA2A1_EK)))
/*******************************************************************************************************************//**
 *  @brief       Performs SHA256 Hashing operation.
 *  @param[IN]   None
 *  @retval      PSA_SUCCESS or any other possible error code
 **********************************************************************************************************************/
psa_status_t sha_operation(void)
{
    psa_status_t         status                         = (psa_status_t)RESET_VALUE;
    psa_algorithm_t      alg                            = PSA_ALG_SHA_256;   // SHA256 algorithm.
    psa_hash_operation_t operation                      = {RESET_VALUE};//operation object to set up for Hash operation.
    size_t               expected_hash_len              = PSA_HASH_SIZE(alg);// expected hash length.
    uint8_t              actual_hash[PSA_HASH_MAX_SIZE] = {RESET_VALUE};     // Buffer where the hash is to be written.
    size_t               actual_hash_len                = RESET_VALUE;   // number of bytes that make up the hash value.
    int                  mbed_ret_val                   = RESET_VALUE;

    /* Buffer containing the message fragment to hash.*/
    const uint8_t sha256_input_data[] =
    {
        0x2e, 0x7e, 0xa8, 0x4d, 0xa4, 0xbc, 0x4d, 0x7c, 0xfb, 0x46, 0x3e, 0x3f, 0x2c, 0x86, 0x47, 0x05,
        0x7a, 0xff, 0xf3, 0xfb, 0xec, 0xec, 0xa1, 0xd2, 00
    };

    /* Buffer containing expected hash.*/
    const uint8_t sha256_expected_data[] =
    {
        0x76, 0xe3, 0xac, 0xbc, 0x71, 0x88, 0x36, 0xf2, 0xdf, 0x8a, 0xd2, 0xd0, 0xd2, 0xd7, 0x6f, 0x0c,
        0xfa, 0x5f, 0xea, 0x09, 0x86, 0xbe, 0x91, 0x8f, 0x10, 0xbc, 0xee, 0x73, 0x0d, 0xf4, 0x41, 0xb9
    };

    APP_PRINT("\r\nHash Operation Started.\r\n");
    /* Set up a multipart hash operation.*/
    status = psa_hash_setup(&operation, alg);
    if (PSA_SUCCESS != status)
    {
        /* Hash setup failed */
        APP_ERR_PRINT("\r\n** psa_hash_setup API FAILED ** \r\n");
        return status;
    }

    /* Add a message fragment to a multipart hash operation.*/
    status = psa_hash_update(&operation, sha256_input_data, sizeof(sha256_input_data));
    if (PSA_SUCCESS != status)
    {
        /* Hash calculation failed */
        APP_ERR_PRINT("\r\n** psa_hash_update API FAILED ** \r\n");
        return status;
    }

    /* Finish the calculation of the hash of a message.*/
    status = psa_hash_finish(&operation, &actual_hash[RESET_VALUE], sizeof(actual_hash), &actual_hash_len);
    if (PSA_SUCCESS != status)
    {
        /* Reading calculated hash failed */
        APP_ERR_PRINT("\r\n** psa_hash_finish API FAILED ** \r\n");
        return status;
    }

    /*compare hash value of calculated value with expected value.*/
    mbed_ret_val = memcmp(&actual_hash[RESET_VALUE], &sha256_expected_data[RESET_VALUE], actual_hash_len);
    if(RESET_VALUE != mbed_ret_val)
    {
        /* Hash compare of calculated value with expected value failed */
        APP_ERR_PRINT ("\r\nHash comparison failed\r\n");
        return mbed_ret_val;
    }

    /*compare hash length of calculated value with expected value.*/
    mbed_ret_val = memcmp(&expected_hash_len, &actual_hash_len, sizeof(expected_hash_len));
    if(RESET_VALUE != mbed_ret_val)
    {
        /* Hash size compare of calculated value with expected value failed */
        APP_ERR_PRINT ("\r\nHash length comparison failed\r\n");
        return mbed_ret_val;
    }

    APP_PRINT("Hash value and Hash length comparison completed successfully.\r\n");
    return status;
}

/*******************************************************************************************************************//**
 *  @brief       Perform ECC -P256R1 Key pair generation, Signing the hashed message with private key
 *               & verify hashed message with public key.
 *  @param[IN]   None
 *  @retval      PSA_SUCCESS or any other possible error code
 **********************************************************************************************************************/
psa_status_t ecc_operation(void)
{
    psa_status_t          status                = (psa_status_t)RESET_VALUE;
    unsigned char         payload_ecc[]         = "ASYMMETRIC_INPUT_FOR_SIGN_ECC"; // Buffer containing message to hash.
    size_t                signature_length                    = RESET_VALUE;               // length of signature.
    psa_key_attributes_t  attributes                          = PSA_KEY_ATTRIBUTES_INIT;   // Contains key attributes.
    psa_key_handle_t      ecc_key_handle                      = {RESET_VALUE};             // ECC Key handle.
    unsigned char         signature[PSA_SIGNATURE_MAX_SIZE]   = {RESET_VALUE};  // Buffer containing signature for ecc.
    uint8_t         payload_hash_ecc[PSA_HASH_MAX_SIZE] = {RESET_VALUE};// Buffer where the hash is to be written.
    uint8_t         ecc_key[ECC_256_EXPORTED_SIZE]      = {RESET_VALUE};// Buffer where the key data is to be written.
    size_t          payload_hash_len_ecc                = RESET_VALUE;  // number of bytes that make up the hash value.
    size_t          ecc_key_length                      = RESET_VALUE;  // number of bytes that make up the key data.

    APP_PRINT("\r\nECC Operation Started.\r\n");

    /* Set Key uses flags, key_algorithm, key_type, key_bits, key_lifetime, key_id*/
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR_WRAPPED(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, ECC_256_BIT_LENGTH);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
    psa_set_key_id(&attributes, ECC_KEY_ID);

    /* Generate ECC P256R1 Key pair */
    status = psa_generate_key(&attributes, &ecc_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_generate_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("ECC Key Pair generated Successfully.\r\n");

    /* Perform Hashing operation.*/
    status = ecc_rsa_hashing_operation(payload_ecc, &payload_hash_ecc[RESET_VALUE], &payload_hash_len_ecc);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** ecc_rsa_hashing_operation failed. ** \r\n");
        return status;
    }

    /* Export the key.*/
    status = psa_export_key(ecc_key_handle, ecc_key, sizeof(ecc_key), &ecc_key_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_export_key API FAILED ** \r\n");
        return status;
    }

    /* Destroy the key and handle */
    status = psa_destroy_key(ecc_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_destroy_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("Exported and Destroyed Key Successfully.\r\n");

    /* Sign message using the private key. */
    /* This is intended to fail as key was not imported after destroying.*/
    status = psa_sign_hash(ecc_key_handle, PSA_ALG_ECDSA(PSA_ALG_SHA_256), payload_hash_ecc, payload_hash_len_ecc,
                           signature, sizeof(signature), &signature_length);
    if (PSA_SUCCESS == status)
    {
        APP_ERR_PRINT("\r\n** psa_sign_hash API Should Fail. ** \r\n");
        return status;
    }
    APP_PRINT("Signing the message failed as key was destroyed.\r\n");

    /* Import the previously exported key pair */
    status = psa_import_key(&attributes, ecc_key, ecc_key_length, &ecc_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_import_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("Imported Key Successfully.\r\n");

    /* Sign message using the private key */
    status = psa_sign_hash(ecc_key_handle, PSA_ALG_ECDSA(PSA_ALG_SHA_256), payload_hash_ecc, payload_hash_len_ecc,
                           signature, sizeof(signature), &signature_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_sign_hash API FAILED ** \r\n");
        return status;
    }

    /* Verify the signature using the public key */
    status = psa_verify_hash(ecc_key_handle, PSA_ALG_ECDSA(PSA_ALG_SHA_256), payload_hash_ecc, payload_hash_len_ecc,
                             signature, signature_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_verify_hash API FAILED ** \r\n");
        return status;
    }

    APP_PRINT("ECC signature validated successfully.\r\n");
    return status;
}

/*******************************************************************************************************************//**
 *  @brief       Perform RSA 2048 Key pair generation, signing the hashed message with private key
 *               & verify hashed message with public key.
 *  @param[IN]   None
 *  @retval      PSA_SUCCESS or any other possible error code
 **********************************************************************************************************************/
psa_status_t rsa_operation(void)
{
    psa_status_t             status                              = (psa_status_t)RESET_VALUE;
    psa_key_handle_t         rsa_key_handle                      = {RESET_VALUE};                   // RSA Key handle.
    unsigned char            payload_rsa[]      = "ASYMMETRIC_INPUT_FOR_SIGN_RSA"; // Buffer containing message to hash.
    unsigned char            signature[PSA_SIGNATURE_MAX_SIZE]   = {RESET_VALUE};// Buffer containing signature for rsa.
    size_t                   signature_length                    = RESET_VALUE;  // length of signature.
    psa_key_attributes_t     attributes                          = PSA_KEY_ATTRIBUTES_INIT; // Contains key attributes.
    uint8_t            rsa_key[RSA_2048_EXPORTED_SIZE]     = {RESET_VALUE};//Buffer where the key data is to be written.
    size_t             rsa_key_len                         = RESET_VALUE;// number of bytes that make up the key data.
    uint8_t            payload_hash_rsa[PSA_HASH_MAX_SIZE] = {RESET_VALUE};// Buffer where the hash is to be written.
    size_t             payload_hash_len_rsa                = RESET_VALUE;// number of bytes that make up the hash value.

    APP_PRINT("\r\nRSA Operation Started.\r\n");

    /* Set Key uses flags, key_algorithm, key_type, key_bits, key_lifetime, key_id*/
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
    psa_set_key_algorithm(&attributes, PSA_ALG_RSA_PKCS1V15_SIGN_RAW);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR_WRAPPED);
    psa_set_key_bits(&attributes, RSA_2048_BIT_LENGTH);
    psa_set_key_id(&attributes, RSA_KEY_ID);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);

    /* Generate RSA 2048 Key pair */
    status = psa_generate_key(&attributes, &rsa_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_generate_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("RSA Key Pair generated Successfully.\r\n");

    /* Perform Hashing operation.*/
    status = ecc_rsa_hashing_operation(payload_rsa, &payload_hash_rsa[RESET_VALUE], &payload_hash_len_rsa);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** ecc_rsa_hashing_operation failed. ** \r\n");
        return status;
    }

    /* export the key*/
    status = psa_export_key(rsa_key_handle, rsa_key, sizeof(rsa_key), &rsa_key_len);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_export_key API FAILED ** \r\n");
        return status;
    }

    /* Destroy key.*/
    status = psa_destroy_key(rsa_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_destroy_key API FAILED ** \r\n");
        return status;
    }
    APP_PRINT("Exported and Destroyed Key Successfully.\r\n");

    /* Sign message using the private key */
    /* This is intended to fail as key was not imported after destroying.*/
    status = psa_sign_hash(rsa_key_handle, PSA_ALG_RSA_PKCS1V15_SIGN_RAW, payload_hash_rsa, payload_hash_len_rsa,
                           signature, sizeof(signature), &signature_length);
    if (PSA_SUCCESS == status)
    {
        APP_ERR_PRINT("\r\n** psa_sign_hash API Should Fail. ** \r\n");
        return status;
    }
    APP_PRINT("Signing the message failed as key was destroyed.\r\n");

    /* Import the key*/
    status = psa_import_key(&attributes, rsa_key, rsa_key_len, &rsa_key_handle);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_import_key API FAILED ** \r\n");
        return status;
    }

    APP_PRINT("Imported Key Successfully.\r\n");
    /* Sign message using the private key */
    status = psa_sign_hash(rsa_key_handle, PSA_ALG_RSA_PKCS1V15_SIGN_RAW, payload_hash_rsa, payload_hash_len_rsa,
                           signature, sizeof(signature), &signature_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_sign_hash API FAILED ** \r\n");
        return status;
    }

    /* Verify the signature using the public key */
    status = psa_verify_hash(rsa_key_handle, PSA_ALG_RSA_PKCS1V15_SIGN_RAW, payload_hash_rsa, payload_hash_len_rsa,
                             signature, signature_length);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_verify_hash API FAILED ** \r\n");
        return status;
    }

    APP_PRINT("RSA signature validated successfully.\r\n");
    return status;
}

/*******************************************************************************************************************//**
 *  @brief       Performs hashing operation for ECC and RSA.
 *  @param[IN]   payload           Buffer containing message to hash.
 *  @param[OUT]  payload_hash      Buffer where the hash is to be written.
 *  @param[OUT]  payload_hash_len  Number of bytes that make up the hash value.
 *  @retval      PSA_SUCCESS or any other possible error code
 **********************************************************************************************************************/
psa_status_t ecc_rsa_hashing_operation(unsigned char * payload, uint8_t * payload_hash, size_t * payload_hash_len)
{
    psa_status_t             status         = (psa_status_t)RESET_VALUE;
    psa_hash_operation_t     hash_operation = {RESET_VALUE}; // operation object to set up for Hash operation.

    /* Calculate the hash of the message*/
    status = psa_hash_setup(&hash_operation, PSA_ALG_SHA_256);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_hash_setup API FAILED ** \r\n");
        return status;
    }

    /*Add a message fragment to a multipart hash operation.*/
    status = psa_hash_update(&hash_operation, payload, ECC_RSA_PAYLOAD_SIZE);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_hash_update API FAILED ** \r\n");
        return status;
    }

    /*Finish the calculation of the hash of a message.*/
    status = psa_hash_finish(&hash_operation, &payload_hash[RESET_VALUE], PSA_HASH_MAX_SIZE, payload_hash_len);
    if (PSA_SUCCESS != status)
    {
        APP_ERR_PRINT("\r\n** psa_hash_finish API FAILED ** \r\n");
    }
    return status;
}

/*******************************************************************************************************************//**
 *  @brief       Initialize Littlefs operation.
 *  @param[IN]   None
 *  @retval      FSP_SUCCESS or any other possible error code
 **********************************************************************************************************************/
fsp_err_t littlefs_init(void)
{
    fsp_err_t          err                       = FSP_SUCCESS;
    int                lfs_err                   = RESET_VALUE;

    /* Open LittleFS Flash port.*/
    err = RM_LITTLEFS_FLASH_Open(&g_rm_littlefs0_ctrl, &g_rm_littlefs0_cfg);
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** RM_LITTLEFS_FLASH_Open API FAILED ** \r\n");
        return err;
    }

    /* Format the filesystem. */
    lfs_err = lfs_format(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
    if(RESET_VALUE != lfs_err)
    {
        APP_ERR_PRINT("\r\n** lfs_format API FAILED ** \r\n");
        deinit_littlefs();
        return (fsp_err_t)lfs_err;
    }

    /* Mount the filesystem. */
    lfs_err = lfs_mount(&g_rm_littlefs0_lfs, &g_rm_littlefs0_lfs_cfg);
    if(RESET_VALUE != lfs_err)
    {
        APP_ERR_PRINT("\r\n** lfs_mount API FAILED ** \r\n");
        deinit_littlefs();
        return (fsp_err_t)lfs_err;
    }
    return err;
}

/*******************************************************************************************************************//**
 *  @brief       De-Initialize the Littlefs.
 *  @param[IN]   None
 *  @retval      None
 **********************************************************************************************************************/
void deinit_littlefs(void)
{
    fsp_err_t          err                       = FSP_SUCCESS;
    /*Closes the lower level driver */
    err = RM_LITTLEFS_FLASH_Close(&g_rm_littlefs0_ctrl);
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\n** RM_LITTLEFS_FLASH_Close API FAILED ** \r\n");
    }
}
#endif

/*******************************************************************************************************************//**
 *  @brief       De-initialize the platform, print and trap error.
 *  @param[IN]   status    error status
 *  @param[IN]   err_str   error string
 *  @retval      None
 **********************************************************************************************************************/
void handle_error(psa_status_t status, char * err_str)
{
    mbedtls_psa_crypto_free();
    /* De-initialize the platform.*/
    mbedtls_platform_teardown(&ctx);
    APP_ERR_PRINT(err_str);
    APP_ERR_TRAP(status);
}

/*******************************************************************************************************************//**
 * @} (end addtogroup crypto_ep)
 **********************************************************************************************************************/
