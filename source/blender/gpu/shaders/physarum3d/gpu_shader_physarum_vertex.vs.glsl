in vec3 v_in_f3Position;

out vec4 v_out_f4Color;



void main()
{
  v_out_f4Color = vec4(v_in_f3Position, 1.0f);
}


//struct VertexInput {
//  float4 position : POSITION;
//  float2 texcoord : TEXCOORD;
//};
//
//struct VertexOutput {
//  float4 position_out : SV_POSITION;
//  float2 texcoord_out : TEXCOORD;
//};
//
//VertexOutput main(VertexInput input)
//{
//  VertexOutput result;
//
//  result.position_out = input.position;
//  result.texcoord_out = input.texcoord;
//
//  return result;
//}
