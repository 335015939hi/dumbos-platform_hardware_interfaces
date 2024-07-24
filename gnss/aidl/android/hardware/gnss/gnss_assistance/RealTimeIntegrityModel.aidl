package android.hardware.gnss.gnss_assistance;

/**
* Contians the real time integrity status of a GNSS satellite.
* TODO: Defined in ?
*/
@VintfStability
parcelable RealTimeIntegrityModel {
    int satelliteId;
    boolean usable;
    long publishDate;
    long startDate;
    long endDate;
    String type;
    String advisory;
}