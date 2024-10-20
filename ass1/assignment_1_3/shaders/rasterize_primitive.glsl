#version 410

// Output for shape id
layout(location = 0) out int shape_id;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;

//Circle and line struct equivalent to the one in shape.h
struct Circle {
    vec4 color;
    vec2 position;
    float radius;
};

struct Line {
    vec2 start_point;
    vec2 end_point;
    vec4 color_left[2];
    vec4 color_right[2];
};

//The uniform buffers for the shapes containing the count of shapes in the first slot and after that, the actual shapes
//We need to give the arrays a fixed size to work with opengl 4.1
layout(std140) uniform circleBuffer
{
    int circle_count;
    Circle circles[32];
} cb;

layout(std140) uniform lineBuffer
{
    int line_count;
    Line lines[800];
} lb;

//The type of the shape we are rasterizing, the same as the enumerator in shapes.h
// 0 - circles
// 1 - lines
// 2 - BezierCurves (Unused)
uniform uint shape_type;

//The maximum distance to the shape for a pixel to be part of the shape
uniform float rasterize_width;

void main()
{
    shape_id = -1;
    vec2 pixel_pos = gl_FragCoord.xy;
    // ---- CIRCLE
    if (shape_type == 0) {
        // Rasterize Circles
        for(int i = 0; i < cb.circle_count; ++i)
        {
            vec2 circlePos = cb.circles[i].position;
            float radius = cb.circles[i].radius;

            // Compute distance from pixel center to circle center
            float dist = distance(pixel_pos, circlePos);

            // Check if within (radius - rasterize_width) to (radius + rasterize_width)
            if(dist >= (radius - rasterize_width) && dist <= (radius + rasterize_width))
            {
                shape_id = i; // Assign the first (lowest index) overlapping circle
                break; // Exit loop since we need the lowest index
            }
        }
    }
    // ---- LINE
    else if (shape_type == 1) {
        for (int i = 0; i < lb.line_count; ++i) {
            Line line = lb.lines[i];
            vec2 line_dir = normalize(line.end_point - line.start_point);
            vec2 line_perpendicular = vec2(-line_dir.y, line_dir.x);
            float dist_to_line = abs(dot(pixel_pos - line.start_point, line_perpendicular));
            if (dist_to_line <= rasterize_width) {
                shape_id = i;
                break;
            }
        }
    }
}