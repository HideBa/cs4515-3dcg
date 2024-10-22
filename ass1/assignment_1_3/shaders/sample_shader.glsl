#version 410

#define M_PI 3.14159265359f

// Output for accumulated color
layout(location = 0) out vec4 outColor;

// Circle and line struct equivalent to the one in shape.h
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

// The uniform buffers for the shapes containing the count of shapes in the
// first slot and after that, the actual shapes
layout(std140) uniform circleBuffer {
  int circle_count;
  Circle circles[32];
}
cb;

layout(std140) uniform lineBuffer {
  int line_count;
  Line lines[800];
}
lb;

// Textures for the rasterized shapes, and the accumulator
uniform isampler2D rasterized_texture;
uniform sampler2D accumulator_texture;

// The type of the shape we are rasterizing, the same as the enumerator in
// shapes.h 0 - circles 1 - lines 2 - BezierCurves (Unused)
uniform uint shape_type;

// The number of samples we have already taken (not directly used here but can
// be for advanced sampling techniques)
uniform uint frame_nr;

// Screen dimensions
uniform ivec2 screen_dimensions;

// Step size for ray-marching
uniform float step_size;

// The maximum amount of raymarching steps we can take
uniform uint max_raymarch_iter;

// Random number generator outputs numbers between [0-1]
float get_random_numbers(inout uint seed) {
  seed = 1664525u * seed + 1013904223u;
  seed += 1664525u * seed;
  seed ^= (seed >> 16u);
  seed += 1664525u * seed;
  seed ^= (seed >> 16u);
  return float(seed) * pow(0.5, 32.0);
}

// Ray marching function
vec2 march_ray(vec2 origin, vec2 direction, float step_size) {
  vec2 current_position = origin;
  for (uint i = 0; i < max_raymarch_iter; ++i) {
    current_position += direction * step_size;

    if (current_position.x < 0.0 ||
        current_position.x >= float(screen_dimensions.x) ||
        current_position.y < 0.0 ||
        current_position.y >= float(screen_dimensions.y)) {
      return vec2(-1.0, -1.0);
    }

    ivec2 texel_coord = ivec2(current_position);
    int shape_idx = texelFetch(rasterized_texture, texel_coord, 0).r;

    if (shape_type == 0 && shape_idx >= 0 && shape_idx < cb.circle_count) {
      return current_position; // Intersection with circle
    } else if (shape_type == 1 && shape_idx >= 0 && shape_idx < lb.line_count) {
      return current_position; // Intersection with line
    }
  }
  return vec2(-1.0, -1.0); // Indicates no hit
}

void main() {
  vec2 ray_origin = vec2(gl_FragCoord.xy);
  uint seed = uint(ray_origin.x) +
              uint(ray_origin.y) * uint(screen_dimensions.x) + frame_nr;

  float angle = 2.0 * M_PI * get_random_numbers(seed);
  vec2 direction = vec2(cos(angle), sin(angle));

  vec2 intersection = march_ray(ray_origin, direction, step_size);

  ivec2 frag_coord = ivec2(ray_origin);
  vec4 previous_accumulator =
      texture(accumulator_texture,
              frag_coord / vec2(screen_dimensions)); // Use texture() to sample

  vec4 new_accumulator = previous_accumulator;

  bool hit = false;
  if (intersection.x != -1.0) {
    hit = true;
  }

  if (hit) {
    // ---- Circle
    if (shape_type == 0) {
      ivec2 texel_coord = ivec2(intersection);
      int shape_idx = texelFetch(rasterized_texture, texel_coord, 0).r;

      if (shape_idx >= 0 && shape_idx < cb.circle_count) {
        Circle current_circle = cb.circles[shape_idx];
        vec4 circle_color = current_circle.color;
        vec2 circle_pos = current_circle.position;
        float radius = current_circle.radius;
        float dist = distance(intersection, circle_pos);

        if (dist < radius) {
          new_accumulator.rgb += circle_color.rgb * circle_color.a;
          new_accumulator.a +=
              circle_color.a; // use later to calculate the average
        }
      }
    }
    // ---- Line
    else if (shape_type == 1) {
      ivec2 texel_cord = ivec2(intersection);
      int shape_idx = texelFetch(rasterized_texture, texel_cord, 0).r;

      if (shape_idx >= 0 && shape_idx < lb.line_count) {
        Line current_line = lb.lines[shape_idx];
        vec2 line_dir = current_line.end_point - current_line.start_point;
        float line_length = length(line_dir);
        vec2 line_dir_norm = normalize(line_dir);
        vec2 to_pixel = intersection - current_line.start_point;

        // Project the intersection point onto the line direction
        float projection = dot(to_pixel, line_dir_norm);

        float dist_to_segment;

        if (projection < 0.0) {
          // Intersection is before the start of the segment
          dist_to_segment = length(to_pixel);
        } else if (projection > line_length) {
          // Intersection is after the end of the segment
          vec2 to_end = intersection - current_line.end_point;
          dist_to_segment = length(to_end);
        } else {
          // Intersection is within the segment
          vec2 closest_point = current_line.start_point + line_dir_norm * projection;
          vec2 perpendicular_vec = intersection - closest_point;
          dist_to_segment = length(perpendicular_vec);
        }

        float weight = 1.0 / (dist_to_segment + 0.001);

        float t = clamp(projection / line_length, 0.0, 1.0);


        vec2 to_origin = ray_origin - current_line.start_point;

        float cross_product = line_dir.x * to_origin.y - line_dir.y * to_origin.x;

        vec4 line_color;
        if (cross_product > 0.0) {
          line_color = mix(current_line.color_left[0], current_line.color_left[1], t);
        } else {
          line_color = mix(current_line.color_right[0], current_line.color_right[1], t);
        }

        new_accumulator.rgb += line_color.rgb * weight;
        new_accumulator.a += weight;
      }
    }
  }
  outColor = new_accumulator;
}
