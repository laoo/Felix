
Texture2DArray<unorm float> font : register( t0 );
Texture2D<uint> text : register( t1 );

RWTexture2D<unorm float4> dst : register( u0 );

[numthreads( 8, 16, 1 )]
void main( uint3 DT : SV_DispatchThreadID, uint3 G : SV_GroupID, uint3 GT : SV_GroupThreadID )
{
  uint idx = text[G.xy];
  float value = 1 - font[int3( GT.xy, idx )];
  dst[DT.xy] = float4( value, value, value, 1 );
}
