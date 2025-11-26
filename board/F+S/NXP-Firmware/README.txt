To be able to boot i.MX8/9 boards, some firmware files from NXP are
required.

- ATF (ARM Trusted Firmware)
- DDR Training Firmware (DDR3L, DDR4, LPDDR4)
- TEE (Trusted Execution Environment, optional)
- HDMI Firmware (i.MX8M)
- SECO (Firmware for SECO chip on i.MX8X)
- SCFW (System Control Firmware for i.MX8X)
- ELE (Firmware for Secure Enclave on i.MX9 and i.MX8ULP)
- UPOWER (Power Management Software for Cortex-M on i.MX8ULP)

Not all files are required for all boards and some boards may need
different files.

None of these files are part of U-Boot. They have to be downloaded
from separate GIT repositories and copied to this directory. The F&S
NBoot build process will then refer to these files here and includes
them in the NBoot image.

Some sources for these firmware files:

- ATF: Binaries included in F&S Releases. Sources:
  F&S Version: git clone https://github.com/FSEmbedded/atf-fus.git
  Original NXP Version: git clone https://github.com/nxp-imx/imx-atf.git
- DDR and HDMI Firmware:
  wget http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-x.y.z.bin
- ELE Firmware:
  wget http://www.freescale.com/lgfiles/NMG/MAD/YOCTO/firmware-ele-imx-x.y.bin
- SCFW and SECO:
  https://www.nxp.com/design/i-mx-developer-resources/i-mx-software-and-development-tools:IMX-SW
