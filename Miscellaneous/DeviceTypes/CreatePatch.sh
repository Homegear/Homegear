rm -f Original/*.xml~
rm -f Patched/*.xml~
diff -Naur Original Patched > DeviceTypePatch.patch
