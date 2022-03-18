# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>
import bpy
from bpy.types import Header, Menu, Panel, PropertyGroup, UIList
from bl_ui.utils import PresetPanel

from bpy.app.translations import pgettext_tip as tip_

class PHYSARUM_HT_header(Header):
    bl_space_type = 'PHYSARUM_EDITOR'

    def draw(self, context):
        layout = self.layout.row()

        layout.template_header()

class PHYSARUM_MT_menu_mode(bpy.types.Menu):
    bl_label = "Display Mode"
    bl_idname = "PHYSARUM_MT_menu_mode"

    def draw(self, context):
        layout = self.layout

        layout.operator("physarum.draw_2d")
        layout.operator("physarum.draw_3d")

class PHYSARUM_PT_mode(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Physarum Display Mode"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.menu("PHYSARUM_MT_menu_mode")

class PHYSARUM_PT_properties(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Physarum properties"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        st = context.space_data

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "particles_population_factor", text="Particles")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "sensor_angle", text="SA")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "sensor_distance", text="SD")
        
        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "rotation_angle", text="RA")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "move_distance", text="MD")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "decay_factor", text="Decay")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "deposit_value")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "spawn_radius")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "center_attraction")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "collision")

class RenderOutputButtonsPanel:
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_context = "output"
    # COMPAT_ENGINES must be defined in each subclass, external engines can add themselves here

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

class PHYSARUM_PT_output(RenderOutputButtonsPanel, Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Render"
    bl_label = "Physarum Render"
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False  # No animation.
        st = context.space_data

        col = layout.column(align=False, heading="Output path")
        layout.prop(st, "output_path", text="")

        layout.use_property_split = True

        col = layout.column(align=False, heading="Render one frame")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.operator("physarum.single_render", text="Render one frame")

        col = layout.column(align=False, heading="Animation length")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "nb_frames_to_render", text="Animation length, in frames")

        col = layout.column(align=False, heading="Render an animation")
        row = col.row(align=True)
        row.operator("physarum.animation_render", text="Render an animation")    

classes = (
    PHYSARUM_HT_header,
    PHYSARUM_MT_menu_mode,
    PHYSARUM_PT_mode,
    PHYSARUM_PT_properties,
    PHYSARUM_PT_output,
)

def register():
    from bpy.utils import register_class
    for cls in classes:
        bpy.utils.register_class(cls)
        
                    
def unregister():
    from bpy.utils import unregister_class
    for cls in classes:
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":  # only for live edit.
    register()
