
Texture2D<unorm float4> src : register( t0 );
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
  for ( int y = 0; y < size; ++y )
  {
    for ( int x = 0; x < size; ++x )
    {
      int dtx = DT.x * rotx.x + DT.y * rotx.y;
      int dty = DT.x * roty.x + DT.y * roty.y;
      dst[off + int2( dtx, dty ) + int2( x, y )]  = src[DT.xy].rgba;
    }
  }
}
