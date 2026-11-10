"""Import the original Godot sprites into the Unreal project as pixel-art textures,
and build the unlit material the backdrop uses. Run headless via -run=pythonscript."""

import os
import unreal

SOURCE_DIR = r"C:\games\baza_tiru\sprites"
DEST_PATH = "/Game/Art"

SPRITES = [
    "background.png",
    "player1.png", "player2.png",
    "asteroid1.png", "asteroid2.png",
    "low_nemy.png", "boss_nemy_01.png",
    "dark1.png", "dark2.png",
    "shot1.png", "shot2.png",
    "explosion.png",
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_textures():
    tasks = []
    for filename in SPRITES:
        full_path = os.path.join(SOURCE_DIR, filename)
        if not os.path.exists(full_path):
            unreal.log_warning("missing sprite: %s" % full_path)
            continue

        task = unreal.AssetImportTask()
        task.filename = full_path
        task.destination_path = DEST_PATH
        task.destination_name = "T_" + os.path.splitext(filename)[0]
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)

    asset_tools.import_asset_tasks(tasks)

    # Pixel art has to stay pixel art: nearest-neighbour sampling, no mip chain
    # softening it at distance, and no block compression chewing up the palette.
    for task in tasks:
        asset_path = "%s/%s" % (DEST_PATH, task.destination_name)
        texture = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not texture:
            unreal.log_warning("did not import: %s" % asset_path)
            continue

        texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        texture.set_editor_property("mip_gen_settings",
                                    unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("compression_settings",
                                    unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("lod_group",
                                    unreal.TextureGroup.TEXTUREGROUP_UI)
        unreal.EditorAssetLibrary.save_asset(asset_path)
        unreal.log("texture ready: %s" % asset_path)


def make_sprite_material(name, masked):
    """An unlit material with one texture parameter, wired straight to emissive."""
    asset_path = "%s/%s" % (DEST_PATH, name)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    material = asset_tools.create_asset(
        name, DEST_PATH, unreal.Material, unreal.MaterialFactoryNew())

    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    if masked:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)

    sampler = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -420, 0)
    sampler.set_editor_property("parameter_name", "Tex")

    default = unreal.EditorAssetLibrary.load_asset("%s/T_background" % DEST_PATH)
    if default:
        sampler.set_editor_property("texture", default)

    unreal.MaterialEditingLibrary.connect_material_property(
        sampler, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    if masked:
        unreal.MaterialEditingLibrary.connect_material_property(
            sampler, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log("material ready: %s" % asset_path)


import_textures()
make_sprite_material("M_BazaBackdrop", masked=False)
make_sprite_material("M_BazaSprite", masked=True)
unreal.log("=== BAZA ART IMPORT DONE ===")
