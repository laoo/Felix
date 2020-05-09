
//Texture2D<unorm float4> src : register( t0 );
RWTexture2D<unorm float4> dst : register( u0 );

cbuffer cb : register( b0 )
{
  int4 posSize;
};


[numthreads( 16, 1, 1 )]
void main( uint3 DT : SV_DispatchThreadID )
{
  
  for ( int y = 0; y < posSize.w; ++y )
  {
    for ( int x = 0; x < posSize.z; ++x )
    {
      dst[posSize.xy + DT.xy * posSize.zw + int2(x,y) ]  = float4( 1, 0, 0, 1 );
    }
  }

}
