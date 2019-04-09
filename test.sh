#!/bin/bash
f1="test1.txt"
f2="test2.txt"
echo "==========================="
echo "======== FAT  TEST ========"
echo "==========================="
echo
echo ">>> COMPILATION : "
make all
echo
echo ">>> CREATE EXAMPLARY FILES BY DD : "
dd if=/dev/urandom of=$f1 bs=100 count=128
dd if=/dev/urandom of=$f2 bs=4096 count=128
echo
echo ">>> CREATE VIRTUAL DISK"
./fatout c 1000000000
echo
echo ">>> DISPLAY DISK PROPERTIES (before copying) : "
./fatout s
echo
echo ">>> COPY FILES TO VIRTUAL DISK : "
./fatout i $f1
./fatout i $f2
echo
echo ">>> DISPLAY DISK PROPERTIES (after copying) : "
./fatout s
echo
echo ">>> DISPLAY FILES"
./fatout m
echo
echo ">>> COPY ORIGIN FILES"
mkdir origin_files
cp $f1 origin_files/$f1
cp $f2 origin_files/$f2
echo
echo ">>> DELETE ORIGIN FILES FROM PHYSIC DISK : "
rm $f1
rm $f2
ls
echo
echo ">>> COPY FILES FROM VIRTUAL DISK : "
./fatout e $f1
./fatout e $f2
ls
echo
echo ">>> DELETE FILES FROM VIRTUAL DISK : "
./fatout x test1.txt
./fatout x test2.txt
echo
echo ">>> DISPLAY DISK PROPERTIES (after deleting) :"
./fatout s
echo
echo ">>> MD5SUM TEST : "
md5sum origin_files/$f1 $f1
md5sum origin_files/$f2 $f2
echo
echo ">>> DELETE VIRTUAL DISK : "
./fatout d
echo
echo ">>> DELETE FILES"
rm origin_files/$f1
rm origin_files/$f2
rmdir --ignore-fail-on-non-empty origin_files
rm $f1
rm $f2
echo "==========================="
echo "========= THE END ========="
echo "==========================="
