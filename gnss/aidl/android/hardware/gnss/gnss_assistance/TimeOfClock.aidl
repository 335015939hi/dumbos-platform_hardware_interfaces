package android.hardware.gnss.gnss_assistance;

/*
 * Contains the time of clock.
 */
@VintfStability
parcelable TimeOfClock {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int seconds;
}