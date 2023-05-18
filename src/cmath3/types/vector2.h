#ifndef vector2_h
#define vector2_h

struct vector2 { float x, y; };

typedef struct vector2 VECTOR2;
typedef struct vector2 *LPVECTOR2;
typedef struct vector2 const *LPCVECTOR2;

void Vector2_set(LPVECTOR2 v, float x, float y);
VECTOR2 Vector2_scale(LPCVECTOR2 v, float s);
float Vector2_dot(LPCVECTOR2 a, LPCVECTOR2 b);
float Vector2_lengthsq(LPCVECTOR2 vec);
float Vector2_len(LPCVECTOR2 vec);

#endif
