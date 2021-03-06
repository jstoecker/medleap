#include "MainConfig.h"

#define CONFIG_FILE_NAME ".medleap"


const std::string MainConfig::WORKING_DIR = "working_dir";
const std::string MainConfig::USE_SRGB = "use_srgb";
const std::string MainConfig::MULTISAMPLING = "multisampling";
const std::string MainConfig::SAMPLES = "samples";
const std::string MainConfig::MIN_SLICES = "min_slices";
const std::string MainConfig::MAX_SLICES = "max_slices";
MainConfig::MainConfig()
{
#if defined(_WIN32)
    std::string homeDir = getenv("HOMEPATH");
#else
    std::string homeDir = getenv("HOME");
#endif
    
    std::string fileName = homeDir + "/" + CONFIG_FILE_NAME;
    if (!load(fileName)) {
        std::cout << "Creating default configuration: " << fileName << std::endl;
        
        // default values
        putValue(WORKING_DIR, homeDir);
        putValue(USE_SRGB, false);
        putValue(MULTISAMPLING, false);
        putValue(SAMPLES, 8);
		putValue(MIN_SLICES, 128);
		putValue(MAX_SLICES, 1024);
        
        save(fileName);
    }
}

MainConfig::~MainConfig()
{
}