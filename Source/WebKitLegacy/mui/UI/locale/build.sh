# in WebKit/OrigynWebBrowser/Api/MorphOS/

target="Catalogs/"

mkdir ${target}/français
mkdir ${target}/polski
mkdir ${target}/español
mkdir ${target}/suomi
mkdir ${target}/svenska
mkdir ${target}/magyar
mkdir ${target}/èe¹tina
mkdir ${target}/italiano
mkdir ${target}/deutsch
mkdir ${target}/russian
mkdir ${target}/norsk

catmaker locale/OWB.cd owb_cat.h CFILE
mv cattmp.c cattmp.cpp

cd locale
flexcat OWB.cd OWB.français.ct CATALOG=${target}/français/OWB.catalog
flexcat OWB.cd OWB.polski.ct CATALOG=${target}/polski/OWB.catalog
flexcat OWB.cd OWB.español.ct CATALOG=${target}/español/OWB.catalog
flexcat OWB.cd OWB.suomi.ct CATALOG=${target}/suomi/OWB.catalog
flexcat OWB.cd OWB.svenska.ct CATALOG=${target}/svenska/OWB.catalog
flexcat OWB.cd OWB.magyar.ct CATALOG=${target}/magyar/OWB.catalog
flexcat OWB.cd OWB.czech.ct CATALOG=${target}/èe¹tina/OWB.catalog
flexcat OWB.cd OWB.italiano.ct CATALOG=${target}/italiano/OWB.catalog
flexcat OWB.cd OWB.deutsch.ct CATALOG=${target}/deutsch/OWB.catalog
flexcat OWB.cd OWB.russian.ct CATALOG=${target}/russian/OWB.catalog
flexcat OWB.cd OWB.norsk.ct CATALOG=${target}/norsk/OWB.catalog
copy OWB.cd Catalogs/
