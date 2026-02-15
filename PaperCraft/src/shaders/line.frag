#version 450

layout(location = 1) flat in uint vSelector;
layout(location = 0) out vec4 outColor;

void main() {



    if(vSelector == 0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);  // black
        }
    else
    {
        outColor = vec4(1.0, 0.0, 0.0, 1.0); // red
    }

    //outColor = vec4(float(vSelector) / 5.0, 0.0, 0.0, 1.0);
    
}


