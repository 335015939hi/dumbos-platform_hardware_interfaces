// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

/**
 * Usage description of a C2Param structure.
 *
 * @ref ParamDescriptor is returned by IConfigurable::querySupportedParams().
 */
@VintfStability
parcelable ParamDescriptor {
    @VintfStability
    @Backing(type="int")
    enum Attrib {
        /**
         * The parameter is required to be specified.
         */
        REQUIRED = 1u << 0,
        /**
         * The parameter retains its value.
         */
        PERSISTENT = 1u << 1,
        /**
         * The parameter is strict.
         */
        STRICT = 1u << 2,
        /**
         * The parameter is publicly read-only.
         */
        READ_ONLY = 1u << 3,
        /**
         * The parameter must not be visible to clients.
         */
        HIDDEN = 1u << 4,
        /**
         * The parameter must not be used by framework (other than testing).
         */
        INTERNAL = 1u << 5,
        /**
         * The parameter is publicly constant (hence read-only).
         */
        CONST = 1u << 6,
    }
    /**
     * Index of the C2Param structure being described.
     */
    int index;
    Attrib attrib;
    /**
     * Name of the structure. This must be unique for each structure.
     */
    String name;
    /**
     * Indices of other C2Param structures that this C2Param structure depends
     * on.
     */
    int[] dependencies;
}
