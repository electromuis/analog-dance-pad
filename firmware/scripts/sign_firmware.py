Import("env")
import zipfile

def package_firmware_esp32s3(source, target, env):
    package_file = env.subst("$BUILD_DIR") + "/firmware.adpf"
    with zipfile.ZipFile(package_file, 'w') as zipf:
        zipf.write(env.subst("$BUILD_DIR") + "/firmware.bin", "firmware.bin")
        zipf.write(env.subst("$BUILD_DIR") + "/bootloader.bin", "bootloader.bin")
        zipf.write(env.subst("$BUILD_DIR") + "/partitions.bin", "partitions.bin")
        zipf.writestr("boardtype.txt", "esp32s3_fsriov3")
    print("Signed firmware package created: " + package_file)

def package_firmware_32u4(source, target, env):
    package_file = env.subst("$BUILD_DIR") + "/firmware.adpf"
    with zipfile.ZipFile(package_file, 'w') as zipf:
        zipf.write(env.subst("$BUILD_DIR") + "/firmware.hex", "firmware.hex")
        zipf.writestr("boardtype.txt", "avr_fsriov2")
    print("Signed firmware package created: " + package_file)
    
env.AddPostAction("$BUILD_DIR/firmware.bin", package_firmware_esp32s3)
env.AddPostAction("$BUILD_DIR/firmware.hex", package_firmware_32u4)