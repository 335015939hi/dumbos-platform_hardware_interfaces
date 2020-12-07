/**
 * Shared Secret definition.
 *
 * An ISharedSecret enables a keymint service to establish a shared secret with one or more other
 * keymint services such as ISecureClock, TEE IKeymintDevice, StringBox IKeymintDevice, etc. The
 * shared secret is a 256-bit HMAC key and it is further used to generate secure tokens with
 * integrity protection. There are two steps to establish a shared secret between collaborating
 * keymint services:
 * Step 1: During Android Startup the system calls each keymint service to get
 * shared secret parameters. This is done using getSharedSecretParameters method defined below. Step
 * 2: The system lexicographically sorts the shared secret parameters received from each keymint
 * service and then sends these sorted parameter list to each keymint service in a
 * computeSharedSecret method defined below.
 */
/**
 * This method is the first step in the process for agreeing on a shared key.  It is called by
 * Android during startup.  The system calls it on each of the HAL instances and collects the
 * results in preparation for the second step.
 *
 * @return The SharedSecretParameters to use.  As specified in the SharedSecretParameters
 *         documentation, the seed must contain the same value in every invocation
 *         of the method on a given device, and the nonce must return the same value for every
 *         invocation during a boot session.
 */

/**
 * This method is the second and final step in the process for agreeing on a shared key.  It is
 * called by Android during startup.  The system calls it on each of the keymint services, and sends
 * to it all of the SharedSecretParameters returned by all keymint services.
 *
 * This method computes the shared 32-byte HMAC ``H'' as follows (all keymint services instances
 * perform the same computation to arrive at the same result):
 *
 *     H = CKDF(key = K,
 *              context = P1 || P2 || ... || Pn,
 *              label = "KeymintSharedMac")
 *
 * where:
 *
 *     ``CKDF'' is the standard AES-CMAC KDF from NIST SP 800-108 in counter mode (see Section
 *           5.1 of the referenced publication).  ``key'', ``context'', and ``label'' are
 *           defined in the standard.  The counter is prefixed and length L appended, as shown
 *           in the construction on page 12 of the standard.  The label string is UTF-8 encoded.
 *
 *     ``K'' is a pre-established shared secret, set up during factory reset.  The mechanism for
 *           establishing this shared secret is implementation-defined.Any method of securely
 *           establishing K that ensures that an attacker cannot obtain or derive its value is
 *           acceptable.
 *
 *           CRITICAL SECURITY REQUIREMENT: All keys created by a IKeymintDevice instance must
 *           be cryptographically bound to the value of K, such that establishing a new K
 *           permanently destroys them.
 *
 *     ``||'' represents concatenation.
 *
 *     ``Pi'' is the i'th SharedSecretParameters value in the params vector. Encoding of an
 *           SharedSecretParameters is the concatenation of its two fields, i.e. seed || nonce.
 *
 * Note that the label "KeymintSharedMac" is the 16-byte UTF-8 encoding of the string.
 *
 * @param an array of SharedSecretParameters The lexicographically sorted SharedSecretParameters
 *        data returned by all keymint services when getSharedSecretParameters was called.
 *
 * @return sharingCheck A 32-byte value used to verify that all the keymint services have
 *         computed the same shared HMAC key.  The sharingCheck value is computed as follows:
 *
 *             sharingCheck = HMAC(H, "Keymint HMAC Verification")
 *
 *         The string is UTF-8 encoded, 27 bytes in length.  If the returned values of all
 *         keymint services don't match, clients must assume that HMAC agreement
 *         failed.
 */
/**
 * SharedSecretParameters holds the data used in the process of establishing a shared secret i.e.
 * HMAC key between multiple keymint services.  These parameters are returned in by
 * getSharedSecretParameters() and send to computeShareSecret().  See the named methods in
 * ISharedSecret for details of usage.
 */
struct HmacSharingParameters {
    /**
     * Either empty or contains a persistent value that is associated with the pre-shared HMAC
     * agreement key.  It is either empty or 32 bytes in length.
     */
    vec<uint8_t> seed;

    /**
     * A 32-byte value which is guaranteed to be different each time
     * getSharedSecretParameters() is called.  Probabilistic uniqueness (i.e. random) is acceptable,
     * though a stronger uniqueness guarantee (e.g. counter) is recommended where possible.
     */
    uint8_t[32] nonce;
};
