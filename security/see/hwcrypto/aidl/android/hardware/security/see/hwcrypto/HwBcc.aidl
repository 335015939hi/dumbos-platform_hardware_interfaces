package android.hardware.security.see.hwcrypto;

interface HwBcc {
    /*
     * get_dice_artifacts() - Retrieves DICE artifacts for a child node in the DICE chain/tree.
     *
     * @context:
     *      Device specific context information used to determine which DICE chain should be
     *      returned if more than 1 exists.
     *
     *
     * Return:
     *      Ok(DiceArtifact) on success, Err(HAlErrorCode) on error.
     */
    DiceArtifactResult get_dice_artifacts(/*@nullable*/ long context);

    /*
     * sign_data() - Retrieves the signed data in a COSE-Sign1 message. Data signed using the CDI
     *               leaf private key. Clients may request to sign using a test key via `test_mode`.
     *
     * @test_mode:
     *      Operation mode which is used to choose if a test key should be used for signing.
     *
     * @SigningAlgorithm:
     *      Signing algorithm to use.
     *
     * @data_to_sign:
     *      Data to be signed.
     *
     * @aad:
     *      additional authenticated data.
     *
     * Return:
     *      Ok(Vector<u8>) containing signed data on success, Err(HAlErrorCode) on error.
     */
    VectorResult sign_data(HwBccMode test_mode, SigningAlgorithm signing_algorithm,
            byte[] data_to_sign, @nullable byte[] aad);

    /*
     * get_bcc() - Retrieves Boot certificate chain (BCC). Clients may request test values using
     *             `test_mode`.
     *
     * @test_mode:
     *      Operation mode which is used to choose if test data should be returned.
     *
     * Return:
     *      Ok(Vector<u8>) containing BCC on success, Err(HAlErrorCode) on error.
     */
    VectorResult get_bcc(HwBccMode test_mode);
}
