$glslc = "C:\Programs\VulkanSDK\1.2.162.1\Bin\glslc.exe"
# From https://antumdeluge.github.io/bin2header.
$bin2h = "C:\Programs\bin2header\bin2header.exe"

&$glslc shaders/shader.vert -o shaders/vert.spv
&$bin2h shaders/vert.spv
&$glslc shaders/shader.frag -o shaders/frag.spv
&$bin2h shaders/frag.spv
