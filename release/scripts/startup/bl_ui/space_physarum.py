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
from bpy.types import Header, Menu, Panel

class PHYSARUM_HT_header(Header):
    bl_space_type = 'PHYSARUM_EDITOR'

    def draw(self, context):
        layout = self.layout.row()

        layout.template_header()

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

        col = layout.column(align=False, heading="Sense Spread")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "sense_spread", text="")

        col = layout.column(align=False, heading="Sense Distance")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "sense_distance", text="")
        
        col = layout.column(align=False, heading="Turn Angle")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "turn_angle", text="")

        col = layout.column(align=False, heading="Move Distance")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "move_distance", text="")

        col = layout.column(align=False, heading="Deposit Value")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "deposit_value", text="")

        col = layout.column(align=False, heading="Decay Factor")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "decay_factor", text="")

        col = layout.column(align=False, heading="Spawn Radius")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "spawn_radius", text="")

        col = layout.column(align=False, heading="Center Attraction")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "center_attraction", text="")

        col = layout.column(align=False, heading="Collision")
        row = col.row(align=True)
        sub = row.row(align=True)
        sub.prop(st, "collision", text="")

classes = (
    PHYSARUM_HT_header,
    PHYSARUM_PT_properties,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
