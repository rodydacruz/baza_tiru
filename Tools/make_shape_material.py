"""Builds M_BazaShape: the lit material every ship, rock and bolt uses.

Directional lights alone leave any face turned away from them at pure black, which
on a faceted rock reads as a hole punched through it. This gives the material an
ambient floor -- a fraction of its own colour fed into emissive -- so an unlit face
falls to a dim version of itself instead of to nothing.
"""

import unreal

DEST_PATH = "/Game/Art"
NAME = "M_BazaShape"
ASSET = "%s/%s" % (DEST_PATH, NAME)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

if unreal.EditorAssetLibrary.does_asset_exist(ASSET):
    unreal.EditorAssetLibrary.delete_asset(ASSET)

mat = asset_tools.create_asset(NAME, DEST_PATH, unreal.Material, unreal.MaterialFactoryNew())

colour = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -620, -40)
colour.set_editor_property("parameter_name", "Color")
colour.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

ambient = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -620, 160)
ambient.set_editor_property("parameter_name", "Ambient")
ambient.set_editor_property("default_value", 0.22)

glow = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -340, 100)
mel.connect_material_expressions(colour, "", glow, "A")
mel.connect_material_expressions(ambient, "", glow, "B")

rough = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -620, 320)
rough.set_editor_property("parameter_name", "Rough")
rough.set_editor_property("default_value", 0.88)

metal = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -620, 440)
metal.set_editor_property("parameter_name", "Metal")
metal.set_editor_property("default_value", 0.0)

mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(ASSET)

print("VERIFY %s vectorparams=%s scalarparams=%s" % (
    NAME,
    [str(p) for p in mel.get_vector_parameter_names(mat)],
    [str(p) for p in mel.get_scalar_parameter_names(mat)]))
