
Texture2D<uint> src : register( t0 );
Texture1D<unorm float4> pal : register( t1 );
RWTexture2D<unorm float4> dst : register( u0 );

cbuffer cb : register( b0 )
{
  int2 rotx;
  int2 roty;
  int2 off;
  int size;
};

[numthreads( 32, 2, 1 )]  
void main( uint3 DT : SV_DispatchThreadID )
{
  uint2 idx = uint2( DT.x / 2, DT.y );
  uint pen;
  if ( DT.x % 2 == 0 )
    pen = src[idx] >> 4;
  else
    pen = src[idx] & 0x0f;

  float4 color = pal[pen].rgba;

  for ( int y = 0; y < size; ++y )
  {
    for ( int x = 0; x < size; ++x )
    {
      int dtx = DT.x * rotx.x + DT.y * rotx.y;
      int dty = DT.x * roty.x + DT.y * roty.y;

      dst[off + int2( dtx, dty ) + int2( x, y )]  = color;
    }
  }
}
