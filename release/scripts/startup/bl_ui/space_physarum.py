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
from bpy.types import Header, Menu, Panel, Operator, Scene
from bpy.props import BoolProperty, IntProperty

Scene.user_stop_physarum_simulation = BoolProperty(
    name = "Stop physarum simulation",
    description ="Let the user to stop the currently running physarum simulation",
    default = False,
)

Scene.physarum_simulation_is_running = BoolProperty(
    name = "Physarum simulation state",
    description ="Describes the physarum simulation state",
    default = False,
)

Scene.physarum_frame_rate = IntProperty(
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

class PHYSARUM_MT_menu_mode(Menu):
    bl_label = "Mode"
    bl_idname = "PHYSARUM_MT_menu_mode"

    def draw(self, context):
        layout = self.layout

        layout.operator("physarum.draw_2d")
        layout.operator("physarum.draw_3d")

class PHYSARUM_PT_simulation(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Simulation"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.menu("PHYSARUM_MT_menu_mode")

        col = layout.column(align=False, heading="")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.label(text="Frame rate")
        sub.prop(context.scene, "physarum_frame_rate", text="")

        col = layout.column(align=False, heading="Start physarum simulation")
        row = col.row(align=True)
        row.operator("physarum.start_simulation", text="Play", icon="PLAY")

        col = layout.column(align=False, heading="Pause physarum simulation")
        row = col.row(align=True)
        row.operator("physarum.stop_simulation", text="Pause", icon="PAUSE")

        col = layout.column(align=False, heading="Reset physarum simulation")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.operator("physarum.reset_physarum", text="Reset", icon="FILE_REFRESH")

class PHYSARUM_PT_properties(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Properties"
    bl_label = "Properties"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False
        st = context.space_data

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "background_color", text="Background color")

        col = layout.column(align=False)
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "particles_color", text="Particles color")

        layout.row()
        layout.row()
        layout.row()

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

        layout.row()
        layout.row()
        layout.row()

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

class PHYSARUM_OT_start_simulation(Operator):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Start Physarum"
    bl_idname = "physarum.start_simulation"
    bl_description = "Start the physarum simulation"

    updating = False
    timer = None

    def modal(self, context, event):
        if event.type == 'TIMER':

            if context.scene.user_stop_physarum_simulation==True:
                self.cancel(context)
                return {'CANCELLED'}

            # Forces to redraw the view (magic trick)
            bpy.context.scene.frame_set(bpy.data.scenes['Scene'].frame_current)

        return {'PASS_THROUGH'}

    def execute(self, context):
        time_step = 1 / context.scene.physarum_frame_rate
        self.timer = context.window_manager.event_timer_add(time_step, window = context.window)
        self.updating = False
        context.scene.user_stop_physarum_simulation=False
        context.scene.physarum_simulation_is_running=True
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def cancel(self, context):
        context.window_manager.event_timer_remove(self.timer)
        context.scene.physarum_simulation_is_running=False
        context.scene.user_stop_physarum_simulation=False
        self.timer = None

    @classmethod
    def poll(cls, context):
        return context.scene.physarum_simulation_is_running == False    

class PHYSARUM_OT_stop_simulation(Operator):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Stop physarum simulation"
    bl_idname = "physarum.stop_simulation"
    bl_description = "Stop the physarum simulation"

    def execute(self, context):
        context.scene.user_stop_physarum_simulation = True
        return {'FINISHED'}

    @classmethod
    def poll(cls, context):
        return context.scene.physarum_simulation_is_running == True

class PHYSARUM_PT_output(Panel):
    bl_space_type = 'PHYSARUM_EDITOR'
    bl_region_type = 'UI'
    bl_category = "Render"
    bl_label = "Render"

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
        sub.operator("physarum.single_render", text="Render one frame", icon="RENDER_STILL")

        col = layout.column(align=False, heading="Duration")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "nb_frames_to_render", text="Duration")

        col = layout.column(align=False, heading="Render an animation")
        row = col.row(align=True)
        row.operator("physarum.animation_render", text="Render an animation", icon="RENDER_ANIMATION")

classes = (
    PHYSARUM_HT_header,
    PHYSARUM_MT_menu_mode,
    PHYSARUM_PT_simulation,
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
