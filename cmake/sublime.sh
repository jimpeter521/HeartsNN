BUILD_DIR=builds/sublime
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR} && cmake -G 'Sublime Text 2 - Ninja' ../..
