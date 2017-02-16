#!/bin/sh

SPE_BE_TEST_PATH=./BE_TEST.`date +%F`
ROOTFS_PATH=../../../rootfs-x86.build
OUTPUT_FILE=./SPE_BE_TEST.`date +%F`.tgz

mkdir -p ${SPE_BE_TEST_PATH}/module
mkdir -p ${SPE_BE_TEST_PATH}/sbin
mkdir -p ${SPE_BE_TEST_PATH}/fw

cp ${ROOTFS_PATH}/sbin/* ${SPE_BE_TEST_PATH}/sbin
cp ${ROOTFS_PATH}/lib/modules/* ${SPE_BE_TEST_PATH}/module
cp -a ${ROOTFS_PATH}/lib/firmware/ath6k/* ${SPE_BE_TEST_PATH}/fw

tar czvf ${OUTPUT_FILE} ${SPE_BE_TEST_PATH}

rm -rf ${SPE_BE_TEST_PATH}

echo "DONE! FILENAME - ${SPE_BE_TEST_PATH}"
