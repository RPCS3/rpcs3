-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}

-keep,includedescriptorclasses class net.rpcs3.RPCS3 { *; }
-keep,includedescriptorclasses class net.rpcs3.ProgressRepository { *; }
-keep,includedescriptorclasses class net.rpcs3.FirmwareRepository { *; }
-keep,includedescriptorclasses class net.rpcs3.GameRepository { *; }
-keep,includedescriptorclasses class net.rpcs3.GameInfo { *; }

-keepattributes Signature,InnerClasses,EnclosingMethod,RuntimeVisibleAnnotations,AnnotationDefault

-keepclassmembers class **$$serializer { *; }
-keepclassmembers @kotlinx.serialization.Serializable class ** {
    *** Companion;
    *** serializer(...);
}
