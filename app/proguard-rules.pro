# Add project specific ProGuard rules here.
#-renamesourcefileattribute SourceFile
#-dontobfuscate
-keepclassmembers class ** {
   public static void Start (***);
}
-keep public class com.envystore.overlay.MainActivity
