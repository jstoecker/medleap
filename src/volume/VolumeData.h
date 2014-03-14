#ifndef __MEDLEAP_VOLUME_DATA__
#define __MEDLEAP_VOLUME_DATA__

#include <GL/glew.h>
#include <vector>
#include <string>
#include "Window.h"
#include "volume/BoundingBox.h"
#include "math/Vector3.h"
#include "gl/Texture.h"
#include "math/Matrix3.h"
#include "gdcmReader.h"
#include "gdcmAttribute.h"
#include "Histogram.h"

/** Volumetric data loaded from a DICOM image series. Stored in a regular grid of voxels. All voxel values are assumed to be an integer  format, 8 or 16 bits, signed or unsigned. */
class VolumeData
{
public:
    
    /** The type of image data */
    enum Modality
    {
        CT,         // computed tomography
        MR,         // magnetic resonance
        UNKNOWN     // any other data
    };
    
    /** Clean up resources */
    ~VolumeData();
    
    /** Dimensions of each voxel in real world units (ex. millimeters) */
    const cgl::Vec3& getVoxelSize() const;
    
    /** Dimensions of the entire volume in millimeters */
    cgl::Vec3 getDimensionsMM() const;
    
    /** Returns a bounding box of the normalized real world dimensions of the volume. */
    const BoundingBox& getBounds();
    
    /** Number of pixels horizontally in each slice */
    unsigned int getWidth();
    
    /** Number of pixels vertically in each slice */
    unsigned int getHeight();
    
    /** Number of slices */
    unsigned int getDepth();
    
    /** Size in bytes of a single slice/image */
    unsigned int getImageSize();
    
    /** Size in bytes of a single pixel */
    unsigned int getPixelSize();
    
    /** Size in bytes of all slices */
    unsigned int getVolumeSize();
    
    /** Total number of voxels in the volume (WxHxD) */
    unsigned int getNumVoxels();
    
    /** Type for channels in the images */
    GLenum getType();
    
    /** Data is stored in a signed format */
    bool isSigned();
    
    /** The minimum voxel value stored in the data */
    int getMinValue();
    
    /** The maximum voxel value stored in the data */
    int getMaxValue();

    /** Returns the modality of the data */
    Modality getModality();
    
    /** Returns the default histogram */
    Histogram* getHistogram();
    
    /** Windows that store values of interest */
    std::vector<Window>& getWindows();
    
    /** The currently active voxel value window (determines which values are displayed) */
    Window& getCurrentWindow();
    
    /** Matrix that transforms DICOM image space (+X right, +Y down) to patient space (+X = left, +Y = posterior, +Z = superior) */
    const cgl::Mat3& getPatientBasis() const;
    
    /** Checks if the DICOM tag <G,E> exists in the data set */
    bool hasValue(uint16_t G, uint16_t E);
    
    /** Returns a value of type T with the DICOM tag <G,E>. */
    template <typename T, uint16_t G, uint16_t E> T getValue() {
        gdcm::Attribute<G,E> at;
        at.SetFromDataSet(reader.GetFile().GetDataSet());
        return at.GetValue();
    }
    
    /** Returns a value of type T with the DICOM tag <G,E>. */
    template <typename T, uint16_t G, uint16_t E> T getValues() {
        gdcm::Attribute<G,E> at;
        at.SetFromDataSet(reader.GetFile().GetDataSet());
        return at.GetValues();
    }
    
    /** Sets the size of each voxel in real world units */
    void setVoxelSize(float x, float y, float z);
    
    /** Sets the current window to the next available window */
    void setNextWindow();
    
    /** Sets the current window to the previous available window */
    void setPrevWindow();
    
    /** Stores the image/slice at depth into the provided 2D texture */
    void loadTexture2D(GLuint& texture, int depth);
    
    /** Stores all images/slices into a 3D texture (single channel per voxel) */
    void loadTexture3D(cgl::Texture* texture);

    
    // raw data buffer
    char* data;
    
private:
    int width;
    int height;
    int depth;
    GLenum format;
    GLenum type;
    int minVoxelValue;
    int maxVoxelValue;
    cgl::Vec3 voxelSize;
    BoundingBox* bounds;
    gdcm::Reader reader;
    Modality modality;
    cgl::Mat3 orientation;
    std::vector<Window> windows;
    int activeWindow;
    Histogram* histogram;
    

    /** Private constructor since loading is complex and done by the Loader class */
    VolumeData();
    
    GLenum internalFormat();
    
    friend class VolumeLoader;
};

#endif // __MEDLEAP_VOLUME_DATA__