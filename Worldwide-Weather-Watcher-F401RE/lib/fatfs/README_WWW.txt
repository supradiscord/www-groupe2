FatFs R0.16 (ChaN) - systeme de fichiers FAT pour la carte SD.
Source : http://elm-chan.org/fsw/ff/ (licence BSD 1 clause, voir LICENSE.txt)
Modifications de ffconf.h pour le projet WWW :
  FF_CODE_PAGE 437 (au lieu de 932) : noms de fichiers en ASCII, moins de memoire
  FF_FS_TINY   1                    : un seul buffer de secteur partage (economise la RAM)
L'interface materielle (diskio.c) est dans lib/cesi_grove/src/diskio.c.
