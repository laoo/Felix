#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Buffer Definitions: 
//
// cbuffer cb
// {
//
//   int2 rotx;                         // Offset:    0 Size:     8
//   int2 roty;                         // Offset:    8 Size:     8
//   int2 off;                          // Offset:   16 Size:     8
//   int size;                          // Offset:   24 Size:     4
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// src                               texture  unorm4          2d             t0      1 
// dst                                   UAV  unorm4          2d             u0      1 
// cb                                cbuffer      NA          NA            cb0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// no Input
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// no Output
cs_5_0
dcl_globalFlags refactoringAllowed
dcl_constantbuffer CB0[2], immediateIndexed
dcl_resource_texture2d (unorm,unorm,unorm,unorm) t0
dcl_uav_typed_texture2d (unorm,unorm,unorm,unorm) u0
dcl_input vThreadID.xy
dcl_temps 5
dcl_thread_group 32, 2, 1
imul null, r0.xyzw, vThreadID.xyxy, cb0[0].xyzw
iadd r0.xyzw, r0.ywww, r0.xzzz
iadd r0.xyzw, r0.xyzw, cb0[1].xyyy
mov r1.xy, vThreadID.xyxx
mov r1.zw, l(0,0,0,0)
ld_indexable(texture2d)(unorm,unorm,unorm,unorm) r1.xyzw, r1.xyzw, t0.xyzw
mov r2.x, l(0)
loop 
  ige r2.y, r2.x, cb0[1].z
  breakc_nz r2.y
  mov r3.yzw, r2.xxxx
  mov r3.x, l(0)
  loop 
    ige r2.y, r3.x, cb0[1].z
    breakc_nz r2.y
    iadd r4.xyzw, r0.xyzw, r3.xyzw
    store_uav_typed u0.xyzw, r4.xyzw, r1.xyzw
    iadd r3.x, r3.x, l(1)
  endloop 
  iadd r2.x, r2.x, l(1)
endloop 
ret 
// Approximately 22 instruction slots used
#endif

const BYTE g_Renderer[] =
{
     68,  88,  66,  67,  98, 121, 
    118,  89,  44, 175, 132, 234, 
    209,  13, 145, 187,  48,   2, 
    245,  38,   1,   0,   0,   0, 
     20,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     44,   2,   0,   0,  60,   2, 
      0,   0,  76,   2,   0,   0, 
    120,   4,   0,   0,  82,  68, 
     69,  70, 240,   1,   0,   0, 
      1,   0,   0,   0, 168,   0, 
      0,   0,   3,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
     83,  67,   0,   1,   0,   0, 
    200,   1,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    156,   0,   0,   0,   2,   0, 
      0,   0,   1,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,  13,   0, 
      0,   0, 160,   0,   0,   0, 
      4,   0,   0,   0,   1,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     13,   0,   0,   0, 164,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
    115, 114,  99,   0, 100, 115, 
    116,   0,  99,  98,   0, 171, 
    164,   0,   0,   0,   4,   0, 
      0,   0, 192,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     96,   1,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
      2,   0,   0,   0, 108,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 144,   1, 
      0,   0,   8,   0,   0,   0, 
      8,   0,   0,   0,   2,   0, 
      0,   0, 108,   1,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 149,   1,   0,   0, 
     16,   0,   0,   0,   8,   0, 
      0,   0,   2,   0,   0,   0, 
    108,   1,   0,   0,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    153,   1,   0,   0,  24,   0, 
      0,   0,   4,   0,   0,   0, 
      2,   0,   0,   0, 164,   1, 
      0,   0,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 114, 111, 
    116, 120,   0, 105, 110, 116, 
     50,   0, 171, 171,   1,   0, 
      2,   0,   1,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    101,   1,   0,   0, 114, 111, 
    116, 121,   0, 111, 102, 102, 
      0, 115, 105, 122, 101,   0, 
    105, 110, 116,   0, 171, 171, 
      0,   0,   2,   0,   1,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 158,   1,   0,   0, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  49, 
     48,  46,  49,   0,  73,  83, 
     71,  78,   8,   0,   0,   0, 
      0,   0,   0,   0,   8,   0, 
      0,   0,  79,  83,  71,  78, 
      8,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
     83,  72,  69,  88,  36,   2, 
      0,   0,  80,   0,   5,   0, 
    137,   0,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   2,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     17,  17,   0,   0, 156,  24, 
      0,   4,   0, 224,  17,   0, 
      0,   0,   0,   0,  17,  17, 
      0,   0,  95,   0,   0,   2, 
     50,   0,   2,   0, 104,   0, 
      0,   2,   5,   0,   0,   0, 
    155,   0,   0,   4,  32,   0, 
      0,   0,   2,   0,   0,   0, 
      1,   0,   0,   0,  38,   0, 
      0,   8,   0, 208,   0,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,   4,   2,   0, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     30,   0,   0,   7, 242,   0, 
     16,   0,   0,   0,   0,   0, 
    214,  15,  16,   0,   0,   0, 
      0,   0, 134,  10,  16,   0, 
      0,   0,   0,   0,  30,   0, 
      0,   8, 242,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 133,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     54,   0,   0,   4,  50,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   0,   2,   0,  54,   0, 
      0,   8, 194,   0,  16,   0, 
      1,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     45,   0,   0, 137, 194,   0, 
      0, 128,  67,  68,   4,   0, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   5,  18,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  48,   0,   0,   1, 
     33,   0,   0,   8,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     10,   0,  16,   0,   2,   0, 
      0,   0,  42, 128,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   4,   3, 
     26,   0,  16,   0,   2,   0, 
      0,   0,  54,   0,   0,   5, 
    226,   0,  16,   0,   3,   0, 
      0,   0,   6,   0,  16,   0, 
      2,   0,   0,   0,  54,   0, 
      0,   5,  18,   0,  16,   0, 
      3,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     48,   0,   0,   1,  33,   0, 
      0,   8,  34,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16,   0,   3,   0,   0,   0, 
     42, 128,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   4,   3,  26,   0, 
     16,   0,   2,   0,   0,   0, 
     30,   0,   0,   7, 242,   0, 
     16,   0,   4,   0,   0,   0, 
     70,  14,  16,   0,   0,   0, 
      0,   0,  70,  14,  16,   0, 
      3,   0,   0,   0, 164,   0, 
      0,   7, 242, 224,  17,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   4,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  30,   0,   0,   7, 
     18,   0,  16,   0,   3,   0, 
      0,   0,  10,   0,  16,   0, 
      3,   0,   0,   0,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     22,   0,   0,   1,  30,   0, 
      0,   7,  18,   0,  16,   0, 
      2,   0,   0,   0,  10,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   1,   0, 
      0,   0,  22,   0,   0,   1, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     22,   0,   0,   0,   5,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0
};