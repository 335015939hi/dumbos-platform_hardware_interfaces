// FIXME: license file, or use the -l option to generate the files with the header.

package android.hardware.media.c2;

import android.hardware.media.c2.IConfigurable;

/**
 * Component interface object. This object contains all of the configurations of
 * a potential or actual component. It can be created and used independently of
 * an actual Codec2 component to query supported parameters for various
 * component settings, and configurations for a potential component.
 *
 * An actual component exposes this interface via IComponent::getInterface().
 */
@VintfStability
interface IComponentInterface {
    // Adding return type to method instead of out param IConfigurable configurable since there is only one return value.
    /**
     * Returns the @ref IConfigurable instance associated to this component
     * interface.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();
}
