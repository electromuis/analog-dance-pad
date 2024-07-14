Import("env")
import zipfile

def package_firmware(env, files):
    envname = env.subst("$PIOENV")
    package_file = env.subst("$BUILD_DIR") + "/" + envname + ".adpf"

    with zipfile.ZipFile(package_file, 'w') as zipf:
        for f in files:
            zipf.write(env.subst("$BUILD_DIR") + "/"+f, f)

        zipf.writestr("boardtype.txt", env.GetProjectOption("board_package_signature"))
    print("Signed firmware package created: " + package_file)

def package_firmware_esp32s3(source, target, env):
    return package_firmware(env, ["firmware.bin", "bootloader.bin", "partitions.bin"])

def package_firmware_32u4(source, target, env):
    return package_firmware(env, ["firmware.hex"])
    
env.AddPostAction("$BUILD_DIR/firmware.bin", package_firmware_esp32s3)
env.AddPostAction("$BUILD_DIR/firmware.hex", package_firmware_32u4)