// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.ParamField;

/**
 * Query information for supported values of a field. This is used as input to
 * IConfigurable::querySupportedValues().
 */
@VintfStability
parcelable FieldSupportedValuesQuery {
    @VintfStability
    @Backing(type="int")
    enum Type {
        /**
         * Query all possible values regardless of other settings.
         */
        POSSIBLE,
        /**
         * Query currently possible values given dependent settings.
         */
        CURRENT,
    }
    /**
     * Identity of the field to query.
     */
    ParamField field;
    /**
     * Type of the query. See #Type for more information.
     */
    Type type;
}
