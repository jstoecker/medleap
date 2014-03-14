#include "VolumeLoader.h"
#include "gdcmImageReader.h"
#include "gdcmAttribute.h"
#include "gdcmTag.h"
#include "gdcmScanner.h"
#include "gdcmIPPSorter.h"
#include "util/Util.h"

using namespace std;
using namespace gdcm;

vector<VolumeLoader::ID> VolumeLoader::search(const char* directoryPath)
{
    vector<ID> ids;
    
    // find all DICOM files in the directory that have a series UID
    Tag uid(0x0020,0x000e);
    Tag modality(0x0008,0x0060);
    Directory directory;
    directory.Load(directoryPath);
    Scanner scanner;
    scanner.AddTag(uid);
    scanner.AddTag(modality);
    scanner.Scan(directory.GetFilenames());
    vector<string> seriesIDs = scanner.GetOrderedValues(uid);
    
    // go through each series and check its type
    for (string seriesID : seriesIDs) {
        vector<string> files = scanner.GetAllFilenamesFromTagToValue(uid, seriesID.c_str());
        
        // a volume must have more than 1 image, so I'm ignoring other series
        if (files.size() > 1) {
            string strModality = scanner.GetValue(files[0].c_str(), modality);
            if (strModality == "CT") {
                ID id = { seriesID, directoryPath, VolumeData::CT, files.size() };
                ids.push_back(id);
            } else if (strModality == "MR") {
                ID id = { seriesID, directoryPath, VolumeData::MR, files.size() };
                ids.push_back(id);
            } else {
                ID id = { seriesID, directoryPath, VolumeData::UNKNOWN, files.size() };
                ids.push_back(id);
            }
        }
    }
    
    return ids;
}

void VolumeLoader::sortFiles(VolumeLoader::ID id, vector<string>& fileNames, double* zSpacing)
{
    // use directory from the series
    Directory directory;
    directory.Load(id.directory);
    
    // scan directory by series UID
    Tag uid(0x0020,0x000e);
    Scanner scanner;
    scanner.AddTag(uid);
    scanner.Scan(directory.GetFilenames());
    
    // sort files by (tolerance is default from GDCM sample code)
    Directory::FilenamesType unsorted = scanner.GetAllFilenamesFromTagToValue(uid, id.uid.c_str());
    
    IPPSorter sorter;
    sorter.SetComputeZSpacing(true);
    sorter.SetZSpacingTolerance(0.001);
    
    // weak error checking
    if (!sorter.Sort(unsorted)) {
        cerr << "Problem sorting images!" << endl;
    }
    
    fileNames = sorter.GetFilenames();
    *zSpacing = sorter.GetZSpacing();
}

VolumeData* VolumeLoader::load(const char* directoryPath)
{
    std::vector<ID> ids = search(directoryPath);
    return (ids.size() == 0 ? NULL : load(ids[0]));
}

VolumeData* VolumeLoader::load(VolumeLoader::ID id)
{
    // sort DCM files so they are ordered correctly along Z
    vector<string> files;
    double zSpacing;
    sortFiles(id, files, &zSpacing);
    if (files.size() < 2)
        return NULL;
    
    
    // using the first image to read dimensions that shouldn't change
    ImageReader reader;
    reader.SetFileName(files[0].c_str());
    reader.Read();
    Image& img = reader.GetImage();
    DataSet& dataSet = reader.GetFile().GetDataSet();
    
    volume = new VolumeData;
    volume->modality = id.modality;
    volume->width = img.GetColumns();
    volume->height = img.GetRows();
    volume->depth = files.size();
    
    
    // only supporting 8/16-bit monochrome images (CT and MR)
    // even if CT data is stored unsigned, it is always signed after the
    // conversion to Hounsfield units (might need to worry about how I do
    // this if the source data has more than 12 out of the 16 bytes used)
    volume->format = GL_RED;
    switch (img.GetPixelFormat())
    {
        case PixelFormat::INT8:
            volume->type = GL_BYTE;
            break;
        case PixelFormat::UINT8:
            volume->type = (id.modality == VolumeData::CT) ? GL_BYTE : GL_UNSIGNED_BYTE;
            break;
        case PixelFormat::INT16:
            volume->type = GL_SHORT;
            break;
        case PixelFormat::UINT16:
            volume->type = (id.modality == VolumeData::CT) ? GL_SHORT : GL_UNSIGNED_SHORT;
            break;
        default:
            delete volume;
            return NULL;
            break;
    }
    
    // now that the type and dimensions are known, allocate memory for voxels
    volume->data = new char[volume->width * volume->height * volume->depth * gl::sizeOf(volume->type)];
    
    // load first image (already in reader memory)
    img.GetBuffer(volume->data);
    gl::flipImage(volume->data, volume->width, volume->height, volume->getPixelSize());
    
    // load all other images
    for (int i = 1; i < volume->depth; i++) {
        size_t offset = i * volume->getImageSize();
        ImageReader reader;
        reader.SetFileName(files[i].c_str());
        reader.Read();
        reader.GetImage().GetBuffer(volume->data + offset);
        gl::flipImage(volume->data + offset, volume->width, volume->height, volume->getPixelSize());
    }
    
    // Apply modality LUT (if possible) and update min/max values
    switch (volume->type)
    {
        case GL_BYTE:
            if (volume->modality == VolumeData::UNKNOWN)
                calculateMinMax<GLbyte>();
            else
                applyModalityLUT<GLbyte>(img.GetSlope(), img.GetIntercept());
            break;
        case GL_UNSIGNED_BYTE:
            if (volume->modality == VolumeData::UNKNOWN)
                calculateMinMax<GLubyte>();
            else
                applyModalityLUT<GLubyte>(img.GetSlope(), img.GetIntercept());
            break;
        case GL_SHORT:
            if (volume->modality == VolumeData::UNKNOWN)
                calculateMinMax<GLshort>();
            else
                applyModalityLUT<GLshort>(img.GetSlope(), img.GetIntercept());
            break;
        case GL_UNSIGNED_SHORT:
            if (volume->modality == VolumeData::UNKNOWN)
                calculateMinMax<GLushort>();
            else
                applyModalityLUT<GLushort>(img.GetSlope(), img.GetIntercept());
            break;
        default:
            return NULL;
            break;
    }
    
    
    
    // store value of interest LUTs as windows
    if (volume->modality != VolumeData::UNKNOWN) {
        int numWindows;
        double* centers;
        double* widths;
        {
            Attribute<0x0028,0x1050> a;
            a.Set(dataSet);
            numWindows = a.GetNumberOfValues();
            centers = new double[numWindows];
            memcpy(centers, a.GetValues(), sizeof(double) * numWindows);
        }
        {
            Attribute<0x0028,0x1051> a;
            a.Set(dataSet);
            widths = new double[numWindows];
            memcpy(widths, a.GetValues(), sizeof(double) * numWindows);
        }
        for (int i = 0; i < numWindows; i++) {
            Window window(volume->type);
            window.setReal(centers[i], widths[i]);
            volume->windows.push_back(window);
        }
        delete[] centers;
        delete[] widths;
        
        // patient orientation
        const double* cosines = img.GetDirectionCosines();
        cgl::Vec3 x(cosines[0], cosines[1], cosines[2]);
        cgl::Vec3 y(cosines[3], cosines[4], cosines[5]);
        cgl::Vec3 z = x.cross(y);
        volume->orientation = cgl::Mat3(x, y, z);
    } else {
        volume->windows.push_back(Window(volume->type));
    }
    
    // give the series a reader to the first image so it can get meta data
    volume->reader.SetFileName(files[0].c_str());
    volume->reader.Read();
    
    
    // Z spacing should be regular between images (this is NOT slice thickness attribute)
    const double* pixelSpacing = volume->getValues<const double*, 0x0028, 0x0030>();
    volume->setVoxelSize(pixelSpacing[0], pixelSpacing[1], zSpacing);
    
    return volume;
}