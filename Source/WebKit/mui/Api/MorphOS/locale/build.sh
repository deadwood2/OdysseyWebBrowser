# in WebKit/OrigynWebBrowser/Api/MorphOS/

target="Catalogs/"

mkdir ${target}/fran�ais
mkdir ${target}/polski
mkdir ${target}/espa�ol
mkdir ${target}/suomi
mkdir ${target}/svenska
mkdir ${target}/magyar
mkdir ${target}/�e�tina
mkdir ${target}/italiano
mkdir ${target}/deutsch
mkdir ${target}/russian
mkdir ${target}/norsk

catmaker locale/OWB.cd owb_cat.h CFILE
mv cattmp.c cattmp.cpp

cd locale
flexcat OWB.cd OWB.fran�ais.ct CATALOG=${target}/fran�ais/OWB.catalog
flexcat OWB.cd OWB.polski.ct CATALOG=${target}/polski/OWB.catalog
flexcat OWB.cd OWB.espa�ol.ct CATALOG=${target}/espa�ol/OWB.catalog
flexcat OWB.cd OWB.suomi.ct CATALOG=${target}/suomi/OWB.catalog
flexcat OWB.cd OWB.svenska.ct CATALOG=${target}/svenska/OWB.catalog
flexcat OWB.cd OWB.magyar.ct CATALOG=${target}/magyar/OWB.catalog
flexcat OWB.cd OWB.czech.ct CATALOG=${target}/�e�tina/OWB.catalog
flexcat OWB.cd OWB.italiano.ct CATALOG=${target}/italiano/OWB.catalog
flexcat OWB.cd OWB.deutsch.ct CATALOG=${target}/deutsch/OWB.catalog
flexcat OWB.cd OWB.russian.ct CATALOG=${target}/russian/OWB.catalog
flexcat OWB.cd OWB.norsk.ct CATALOG=${target}/norsk/OWB.catalog
copy OWB.cd Catalogs/
