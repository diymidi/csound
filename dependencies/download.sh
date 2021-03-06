#!/bin/sh

flac_src="http://sourceforge.net/projects/flac/files/flac-src/flac-1.2.1-src/flac-1.2.1.tar.gz"
ogg_src="http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz"
vorbis_src="http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz"
libsndfile_src="http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz"
liblo_src="http://sourceforge.net/projects/liblo/files/liblo/0.26/liblo-0.26.tar.gz"
#wiiuse_src="http://sourceforge.net/projects/wiiuse/files/wiiuse/v0.12/wiiuse_v0.12_src.tar.gz"
wiiuse_src="http://codemist.co.uk/jpff/wiiuse_v0.13a.zip"
#portaudio_src="http://www.portaudio.com/archives/pa_stable_v19_20111121.tgz"
portaudio_src="http://portaudio.com/archives/pa_snapshot.tgz"
#portmidi_src="http://sourceforge.net/projects/portmedia/files/portmidi/217/portmidi-src-217.zip"
boost_src="http://sourceforge.net/projects/boost/files/boost/1.52.0/boost_1_52_0.tar.gz"
#fltk_src="http://ftp.easysw.com/pub/fltk/1.3.1/fltk-1.3.1-source.tar.gz"
fltk_src="http://fltk.org/pub/fltk/1.3.2/fltk-1.3.2-source.tar.gz"
#png_src="ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng-1.5.13.tar.gz"
png_src="http://sourceforge.net/projects/libpng/files/libpng16/1.6.6/libpng-1.6.6.tar.gz"
fluidsynth_src="http://sourceforge.net/projects/fluidsynth/files/older%20releases/fluidsynth-1.0.9.tar.gz"
luajit_src="http://luajit.org/download/LuaJIT-2.0.2.tar.gz"
packages=($portaudio_src $portmidi_src $flac_src $ogg_src $vorbis_src $libsndfile_src $liblo_src $wiiuse_src $fltk_src $png_src $boost_src $fluidsynth_src $luajit_src)


#prepare
mkdir cache
#rm -rf build
mkdir build


cd cache
for i in ${packages[*]}; do
  echo $i
  wget -nc $i 
done

cd ../build
for i in `ls ../cache | grep zip`; do
  unzip ../cache/$i 
done

for i in `ls ../cache | grep -v zip`; do
  tar xzvf ../cache/$i 
done

svn co https://portmedia.svn.sourceforge.net/svnroot/portmedia/portmidi/trunk portmidi-svn

cp -r flac-1.2.1 flac-1.2.1-i386
