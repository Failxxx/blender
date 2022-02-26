
uniform sampler2D u_s2Data;

in vec2 v_out_f2Uv;

void main(){
    vec4 f4Source = texture2D(u_s2Data, v_out_f2Uv);
    gl_FragColor = vec4(f4Source.ggg, 1.0f);
}