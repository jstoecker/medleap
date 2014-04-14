#ifndef __medleap__BoundingBox__
#define __medleap__BoundingBox__

#include <vector>
#include <utility>
#include "gl/math/Vector4.h"
#include "gl/math/Vector2.h"

/** Contains vertex positions and edges (indices) for an axis-aligned bounding box */
class BoundingBox
{
public:
    typedef std::pair<unsigned short, unsigned short> Edge;
    
    BoundingBox(float width, float height, float depth);
    
    float getWidth() const;
    float getHeight() const;
    float getLength() const;
    Vec3 getMinimum() const;
    Vec3 getMaximum() const;
    const std::vector<Vec4>& getVertices() const;
    const std::vector<Edge>& getEdges() const;
    
private:
    Vec3 size;
    std::vector<Vec4> vertices;
    std::vector<Edge> edges;
};

#endif /* defined(__medleap__BoundingBox__) */
