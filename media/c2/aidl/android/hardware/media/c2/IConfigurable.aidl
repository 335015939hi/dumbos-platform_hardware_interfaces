// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.FieldSupportedValuesQuery;
import android.hardware.media.c2.FieldSupportedValuesQueryResult;
import android.hardware.media.c2.ParamDescriptor;
import android.hardware.media.c2.SettingResult;
import android.hardware.media.c2.Status;

/**
 * Generic configuration interface presented by all configurable Codec2 objects.
 *
 * This interface must be supported in all states of the owning object, and must
 * not change the state of the owning object.
 */
@VintfStability
interface IConfigurable {
    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Sets a set of parameters for the object.
     *
     * Tuning is performed at best effort: the object must update all supported
     * configurations at best effort and skip unsupported parameters. Any errors
     * are communicated in the return value and in @p failures.
     *
     * A non-strict parameter update with an unsupported value shall cause an
     * update to the closest supported value. A strict parameter update with an
     * unsupported value shall be skipped and a failure shall be returned.
     *
     * If @p mayBlock is false, this method must not block. An update that
     * requires blocking shall be skipped and a failure shall be returned.
     *
     * If @p mayBlock is true, an update may block, but the whole method call
     * has to complete in a timely manner, or `status = TIMED_OUT` is returned.
     *
     * The final values for all parameters set are propagated back to the caller
     * in @p params.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `status = TIMED_OUT` is returned.
     *
     * @note Parameter tuning @e does depend on the order of the tuning
     * parameters, e.g., some parameter update may enable some subsequent
     * parameter update.
     *
     * @param inParams Requested parameter updates.
     * @param mayBlock Whether this call may block or not.
     * @param out status Status of the call, which may be
     *   - `OK`        - All parameters could be updated successfully.
     *   - `BAD_INDEX` - All supported parameters could be updated successfully,
     *                   but some parameters were not supported.
     *   - `NO_MEMORY` - Some supported parameters could not be updated
     *                   successfully because they contained unsupported values.
     *                   These are returned in @p failures.
     *   - `BLOCKING`  - Setting some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out failures List of update failures.
     * @param out outParams Flattened representation of configured parameters. The
     *     order of parameters in @p outParams is based on the order of
     *     requested updates in @p inParams.
     *
     * @sa SettingResult.
     */
    void config(in byte[] inParams, in boolean mayBlock,
        out Status status, out SettingResult[] failures, out byte[] outParams);

    // Adding return type to method instead of out param int id since there is only one return value.
    /**
     * Returns the id of the object. This must be unique among all objects of
     * the same type hosted by the same store.
     *
     * @return Id of the object.
     */
    int getId();

    // Adding return type to method instead of out param String name since there is only one return value.
    /**
     * Returns the name of the object.
     *
     * This must match the name that was supplied during the creation of the
     * object.
     *
     * @return Name of the object.
     */
    String getName();

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Queries a set of parameters from the object.
     *
     * Querying is performed at best effort: the object must query all supported
     * parameters and skip unsupported ones (which may include parameters that
     * could not be allocated). Any errors are communicated in the return value.
     *
     * If @p mayBlock is false, this method must not block. All parameter
     * queries that require blocking must be skipped.
     *
     * If @p mayBlock is true, a query may block, but the whole method call
     * has to complete in a timely manner, or `status = TIMED_OUT` is returned.
     *
     * If @p mayBlock is false, this method must not block. Otherwise, this
     * method is allowed to block for a certain period of time before completing
     * the operation. If the operation is not completed in a timely manner,
     * `status = TIMED_OUT` is returned.
     *
     * @note The order of C2Param objects in @p param does not depend on the
     *     order of C2Param structure indices in @p indices.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released. This call must not change the state nor the
     * internal configuration of the component.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `status = TIMED_OUT` is returned.
     *
     * @param indices List of C2Param structure indices to query.
     * @param mayBlock Whether this call may block or not.
     * @param out status Status of the call, which may be
     *   - `OK`        - All parameters could be queried.
     *   - `BAD_INDEX` - All supported parameters could be queried, but some
     *                   parameters were not supported.
     *   - `NO_MEMORY` - Could not allocate memory for a supported parameter.
     *   - `BLOCKING`  - Querying some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out params Flattened representation of C2Param objects.
     *
     * @sa Params.
     */
    void query(in int[] indices, in boolean mayBlock, out Status status, out byte[] params);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Returns a list of supported parameters within a selected range of C2Param
     * structure indices.
     *
     * @param start The first index of the selected range.
     * @param count The length of the selected range.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `NO_MEMORY` - Not enough memory to complete this method.
     * @param out params List of supported parameters in the selected range. This
     *     list may have fewer than @p count elements if some indices in the
     *     range are not supported.
     *
     * @sa ParamDescriptor.
     */
    void querySupportedParams(in int start, in int count,
        out Status status, out ParamDescriptor[] params);

    // FIXME: AIDL has built-in status types. Do we need the status type here?
    /**
     * Retrieves the supported values for the queried fields.
     *
     * The object must process all fields queried even if some queries fail.
     *
     * If @p mayBlock is false, this method must not block. Otherwise, this
     * method is allowed to block for a certain period of time before completing
     * the operation. If the operation cannot be completed in a timely manner,
     * `status = TIMED_OUT` is returned.
     *
     * \par For IComponent
     *
     * When the object type is @ref IComponent, this method must be supported in
     * any state except released.
     *
     * The blocking behavior of this method differs among states:
     *   - In the stopped state, this must be non-blocking. @p mayBlock is
     *     ignored. (The method operates as if @p mayBlock was false.)
     *   - In any of the running states, this method may block momentarily if
     *     @p mayBlock is true. However, if the call cannot be completed in a
     *     timely manner, `status = TIMED_OUT` is returned.
     *
     * @param inFields List of field queries.
     * @param mayBlock Whether this call may block or not.
     * @param out status Status of the call, which may be
     *   - `OK`        - The operation completed successfully.
     *   - `BLOCKING`  - Querying some parameters requires blocking, but
     *                   @p mayBlock is false.
     *   - `NO_MEMORY` - Not enough memory to complete this method.
     *   - `BAD_INDEX` - At least one field was not recognized as a component
     *                   field.
     *   - `BLOCKING`  - Querying some fields requires blocking, but @p mayblock
     *                   is false.
     *   - `TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `CORRUPTED` - Some unknown error occurred.
     * @param out outFields List of supported values and results for the
     *     supplied queries.
     *
     * @sa FieldSupportedValuesQuery, FieldSupportedValuesQueryResult.
     */
    void querySupportedValues(in FieldSupportedValuesQuery[] inFields, in boolean mayBlock,
        out Status status, out FieldSupportedValuesQueryResult[] outFields);
}
