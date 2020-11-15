grammar UShader;

shader : 'Shader' shader_name '{' hlsl root_signature property_block? pass+ '}';

shader_name : StringLiteral;

hlsl : 'HLSL' ':' StringLiteral;

property_block : 'Properties' '{' property+ '}';

property
	: property_bool
	| property_int
	| property_uint
	| property_float
	| property_double
	| property_bool2
	| property_bool3
	| property_bool4
	| property_int2
	| property_int3
	| property_int4
	| property_uint2
	| property_uint3
	| property_uint4
	| property_float2
	| property_float3
	| property_float4
	| property_double2
	| property_double3
	| property_double4
	| property_2D
	| property_cube
	| property_rgb
	| property_rgba
	;

property_bool : property_name '(' display_name ',' 'bool' ')' ':' val_bool;
property_int : property_name '(' display_name ',' 'int' ')' ':' val_int;
property_uint : property_name '(' display_name ',' 'uint' ')' ':' val_uint;
property_float : property_name '(' display_name ',' 'float' ')' ':' val_float;
property_double : property_name '(' display_name ',' 'double' ')' ':' val_double;
property_bool2 : property_name '(' display_name ',' 'bool2' ')' ':' val_bool2;
property_bool3 : property_name '(' display_name ',' 'bool3' ')' ':' val_bool3;
property_bool4 : property_name '(' display_name ',' 'bool4' ')' ':' val_bool4;
property_int2 : property_name '(' display_name ',' 'int2' ')' ':' val_int2;
property_int3 : property_name '(' display_name ',' 'int3' ')' ':' val_int3;
property_int4 : property_name '(' display_name ',' 'int4' ')' ':' val_int4;
property_uint2 : property_name '(' display_name ',' 'uint2' ')' ':' val_uint2;
property_uint3 : property_name '(' display_name ',' 'uint3' ')' ':' val_uint3;
property_uint4 : property_name '(' display_name ',' 'uint4' ')' ':' val_uint4;
property_float2 : property_name '(' display_name ',' 'float2' ')' ':' val_float2;
property_float3 : property_name '(' display_name ',' 'float3' ')' ':' val_float3;
property_float4 : property_name '(' display_name ',' 'float4' ')' ':' val_float4;
property_double2 : property_name '(' display_name ',' 'double2' ')' ':' val_double2;
property_double3 : property_name '(' display_name ',' 'double3' ')' ':' val_double3;
property_double4 : property_name '(' display_name ',' 'double4' ')' ':' val_double4;
property_2D : property_name '(' display_name ',' '2D' ')' ':' val_tex2d;
property_cube : property_name '(' display_name ',' 'Cube' ')' ':' val_texcube;
property_rgb : property_name '(' display_name ',' 'Color3' ')' ':' val_float3;
property_rgba : property_name '(' display_name ',' 'Color4' ')' ':' val_float4;

val_bool : BooleanLiteral;
val_int : Sign? IntegerLiteral;
val_uint : IntegerLiteral;
val_float
	: Sign ? IntegerLiteral
	| Sign ? FloatingLiteral
	;
val_double
	: Sign ? IntegerLiteral
	| Sign ? FloatingLiteral
	;
val_bool2 : '(' val_bool ',' val_bool ')';
val_bool3 : '(' val_bool ',' val_bool ',' val_bool ')';
val_bool4 : '(' val_bool ',' val_bool ',' val_bool ',' val_bool ')';
val_int2 : '(' val_int ',' val_int ')';
val_int3 : '(' val_int ',' val_int ',' val_int ')';
val_int4 : '(' val_int ',' val_int ',' val_int ',' val_int ')';
val_uint2 : '(' val_uint ',' val_uint ')';
val_uint3 : '(' val_uint ',' val_uint ',' val_uint ')';
val_uint4 : '(' val_uint ',' val_uint ',' val_uint ',' val_uint ')';
val_float2 : '(' val_float ',' val_float ')';
val_float3 : '(' val_float ',' val_float ',' val_float ')';
val_float4 : '(' val_float ',' val_float ',' val_float ',' val_float ')';
val_double2 : '(' val_double ',' val_double ')';
val_double3 : '(' val_double ',' val_double ',' val_double ')';
val_double4 : '(' val_double ',' val_double ',' val_double ',' val_double ')';
val_tex2d
	: default_texture_2d
	| StringLiteral
	;
default_texture_2d
	: 'White'
	| 'Black'
	| 'Bump'
	;
val_texcube
	: default_texture_cube
	| StringLiteral
	;
default_texture_cube
	: 'White'
	| 'Black'
	;

property_name : ID;
display_name : StringLiteral;

root_signature : 'RootSignature' '{' root_parameter+ '}';
root_parameter : RootDescriptorType ('[' register_num ']')? ':' register_index;

register_index
	: shader_register
	| '(' shader_register ',' register_space ')'
	;
shader_register : IntegerLiteral;
register_space : IntegerLiteral;
register_num : IntegerLiteral;

pass : 'Pass' '(' vs ',' ps ')'  '{' pass_statement*'}';
vs : ID;
ps : ID;
pass_statement
	: render_state_setup
	| tags
	| queue
	;
tags : 'Tags' '{' tag+ '}';
tag : StringLiteral ':' StringLiteral;
render_state_setup
	: fill
	| cull
	| ztest
	| zwrite_off
	| blend
	| blend_op
	| color_mask
	| stencil
	;
fill : 'Fill' FillMode;
cull : 'Cull' CullMode;
ztest : 'ZTest' Comparator;
zwrite_off : 'ZWriteOff';
blend : 'Blend' ('['index']')? blend_expr?
	;
blend_expr
	: '('  blend_src_factor_color ',' blend_dst_factor_color ')'
	| '('  blend_src_factor_color ',' blend_dst_factor_color',' blend_src_factor_alpha ',' blend_dst_factor_alpha ')'
	;
index : IntegerLiteral;
blend_src_factor_color : blend_factor;
blend_dst_factor_color : blend_factor;
blend_src_factor_alpha : BlendFactorAlpha;
blend_dst_factor_alpha : BlendFactorAlpha;
blend_factor
	: BlendFactorAlpha
	| 'SrcColor'
	| 'DstColor'
	| 'OneMinusSrcColor'
	| 'OneMinusDstColor'
	;
BlendFactorAlpha
	: 'Zero'
	| 'One'
	| 'SrcAlpha'
	| 'DstAlpha'
	| 'OneMinusSrcAlpha'
	| 'OneMinusDstAlpha'
	;
blend_op
	: 'BlendOp' ('[' index ']')? blend_op_color
	| 'BlendOp' ('[' index ']')? '(' blend_op_color ',' blend_op_alpha ')'
	;
blend_op_color : BlendOpEnum;
blend_op_alpha : BlendOpEnum;
BlendOpEnum
	: 'Add'
	| 'Sub'
	| 'RevSub'
	| 'Min'
	| 'Max'
	;
color_mask : 'ColorMask' ('[' index ']')? color_mask_value;
color_mask_value
	: IntegerLiteral
	| ColorMask_RGBA
	;

stencil : 'Stencil' '{' stencil_state_setup+ '}';
stencil_state_setup
	: stencil_ref
	| stencil_mask_read
	| stencil_mask_write
	| stencil_compare
	| stencil_pass
	| stencil_fail
	| stencil_zfail
	;
stencil_ref : 'Ref' IntegerLiteral;
stencil_mask_read : 'ReadMask' IntegerLiteral;
stencil_mask_write : 'WriteMask' IntegerLiteral;
stencil_compare : 'Comp' Comparator;
stencil_pass : 'Pass' stencil_op;
stencil_fail : 'Fail' stencil_op;
stencil_zfail : 'ZFail' stencil_op;

stencil_op
	: 'Keep'
	| 'Zero'
	| 'Replace'
	| 'IncrSat'
	| 'DecrSat'
	| 'Invert'
	| 'IncrWrap'
	| 'DecrWarp'	
	;

queue : 'Queue' val_queue;
val_queue
	: queue_key
	| IntegerLiteral
	| queue_key Sign IntegerLiteral
	;
queue_key
	: 'Background'
	| 'Geometry'
	| 'AlphaTest'
	| 'Transparent'
	| 'Overlay'
	;
	
StringLiteral : '"' (~'"')* '"';

Comparator
	: 'Less'
	| 'Greater'
	| 'LEqual'
	| 'GEqual'
	| 'Equal'
	| 'NotEqual'
	| 'Always'
	| 'Never'
	;

BooleanLiteral
	: 'false'
	| 'true'
	;

FillMode
	: 'Wireframe'
	| 'Solid'
	;

CullMode
	: 'Front'
	| 'Back'
	| 'Off'
	;

RootDescriptorType
	: 'CBV'
	| 'SRV'
	| 'UAV'
	;

ColorMask_RGBA : [RGBA]+;

IntegerLiteral
	: DecimalLiteral
	| OctalLiteral
	| HexadecimalLiteral
	| BinaryLiteral
	;

FloatingLiteral
	: [0-9]+ '.' [0-9]+ ExponentPart?
	| [0-9]+ ExponentPart?
	;

ExponentPart : [eE] Sign? [0-9]+;

DecimalLiteral
	: '0'
	| [1-9][0-9]*
	;

OctalLiteral : '0'[1-7][0-7]+;
HexadecimalLiteral : ('0x' | '0X') [0-9a-fA-F]+;
BinaryLiteral : ('0b' | '0B') [01]+;

Sign : [+-];

ID : [_a-zA-Z][_0-9a-zA-Z]*;

Whitespace
   : [ \t]+ -> skip
   ;

Newline
   : ('\r' '\n'? | '\n') -> skip
   ;

BlockComment
   : '/*' .*? '*/' -> skip
   ;

LineComment
   : '//' ~ [\r\n]* -> skip
   ;