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

bpy.types.Scene.user_stop_physarum_simulation = bpy.props.BoolProperty(
    name = "Stop physarum simulation",
    description ="Let the user to stop the currently running physarum simulation",
    default = False,
)

bpy.types.Scene.physarum_frame_rate = bpy.props.IntProperty(
    name = "Physarum frame rate",
    description = "Choose a custom frame rate for the physarum simulation",
    default = 60,
    max = 120,
    min = 1,
    step = 1,
)


class PHYSARUM_HT_header(Header):
    bl_space_type = 'PHYSARUM_EDITOR'

    def draw(self, context):
        layout = self.layout.row()

        layout.template_header()

class PHYSARUM_MT_menu_mode(bpy.types.Menu):
    bl_label = "Mode"
    bl_idname = "PHYSARUM_MT_menu_mode"

    def draw(self, context):
        layout = self.layout

        layout.operator("physarum.draw_2d")
        layout.operator("physarum.draw_3d")

class PHYSARUM_PT_mode(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Physarum simulation mode"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.menu("PHYSARUM_MT_menu_mode")

        col = layout.column(align=False, heading="Frame rate")
        row = col.row(align=True)
        sub = row.row(align=True)
        row.prop(context.scene, "physarum_frame_rate", text="Frame rate")

        col = layout.column(align=False, heading="Start physarum simulation")
        row = col.row(align=True)
        row.operator("physarum.start_simulation", text="Start")

        col = layout.column(align=False, heading="Start physarum simulation")
        row = col.row(align=True)
        row.operator("physarum.stop_simulation", text="Stop")

        col = layout.column(align=False, heading="Reset physarum simulation")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.operator("physarum.reset_physarum", text="Reset")

class PHYSARUM_PT_properties(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Physarum simulation properties"

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
        sub.prop(st, "deposit_value", text="Deposit")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "spawn_radius", text="Spawn radius")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "center_attraction", text="Center attraction")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "collision", text="Use collisions")

class PHYSARUM_OT_start_simulation(bpy.types.Operator):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Start Physarum"
    bl_idname = "physarum.start_simulation"

    updating = False
    timer = None

    def modal(self, context, event):
        if event.type == 'TIMER' and not self.updating:
            self.updating = True
            # Forces to redraw the view (magic trick)
            bpy.context.scene.frame_set(bpy.data.scenes['Scene'].frame_current)
            self.updating = False
        if event.type == 'TIMER' and context.scene.user_stop_physarum_simulation==True:
            self.cancel(context)
        return {'PASS_THROUGH'}

    def execute(self, context):
        context.window_manager.modal_handler_add(self)
        self.updating = False
        time = 1 / context.scene.physarum_frame_rate
        self.timer = context.window_manager.event_timer_add(time, window = context.window)
        context.scene.user_stop_physarum_simulation=False
        return {'RUNNING_MODAL'}

    def cancel(self, context):
        if context.scene.user_stop_physarum_simulation==True:
            context.window_manager.event_timer_remove(self.timer)
            self.timer = None
            context.scene.user_stop_physarum_simulation==False
        return {'FINISHED'}

class PHYSARUM_OT_stop_simulation(bpy.types.Operator):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Stop physarum simulation"
    bl_idname = "physarum.stop_simulation"

    def execute(self, context):
        context.scene.user_stop_physarum_simulation = True
        return {'FINISHED'}

class PHYSARUM_PT_output(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Render"
    bl_label = "Physarum Render"

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

        col = layout.column(align=False, heading="Duration")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "nb_frames_to_render", text="Duration")

        col = layout.column(align=False, heading="Render an animation")
        row = col.row(align=True)
        row.operator("physarum.animation_render", text="Render an animation")    

classes = (
    PHYSARUM_HT_header,
    PHYSARUM_MT_menu_mode,
    PHYSARUM_PT_mode,
    PHYSARUM_PT_properties,
    PHYSARUM_OT_start_simulation,
    PHYSARUM_OT_stop_simulation,
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
