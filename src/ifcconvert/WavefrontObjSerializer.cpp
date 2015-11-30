/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include <limits>
#include <iomanip>

#include "../ifcgeom/IfcGeomRenderStyles.h"

#include "WavefrontObjSerializer.h"

bool WaveFrontOBJSerializer::ready() {
	return obj_stream.is_open() && mtl_stream.is_open();
}

void WaveFrontOBJSerializer::writeHeader() {
	obj_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << "\n";
#ifdef WIN32
	const char dir_separator = '\\';
#else
	const char dir_separator = '/';
#endif
	std::string mtl_basename = mtl_filename;
	std::string::size_type slash = mtl_basename.find_last_of(dir_separator);
	if (slash != std::string::npos) {
		mtl_basename = mtl_basename.substr(slash+1);
	}
	obj_stream << "mtllib " << mtl_basename << "\n";
	mtl_stream << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << "\n";	
}

void WaveFrontOBJSerializer::writeMaterial(const IfcGeom::Material& style) {			
	mtl_stream << "newmtl " << style.name() << "\n";
	if (style.hasDiffuse()) {
		const double* diffuse = style.diffuse();
		mtl_stream << "Kd " << diffuse[0] << " " << diffuse[1] << " " << diffuse[2] << "\n";
	}
	if (style.hasSpecular()) {
		const double* specular = style.specular();
		mtl_stream << "Ks " << specular[0] << " " << specular[1] << " " << specular[2] << "\n";
	}
	if (style.hasSpecularity()) {
		mtl_stream << "Ns " << style.specularity() << "\n";
	}
	if (style.hasTransparency()) {
		const double transparency = 1.0 - style.transparency();
		if (transparency < 1) {
			mtl_stream << "Tr " << transparency << "\n";
			mtl_stream << "d "  << transparency << "\n";
			mtl_stream << "D "  << transparency << "\n";
		}
	}
}
void WaveFrontOBJSerializer::write(const IfcGeom::TriangulationElement<double>* o) {

	obj_stream << "g " << o->unique_id() << "\n";
	obj_stream << "s 1" << "\n";

	obj_stream << std::setprecision(std::numeric_limits<double>::digits10);

	const IfcGeom::Representation::Triangulation<double>& mesh = o->geometry();
	
	const int vcount = mesh.verts().size() / 3;
	for ( std::vector<double>::const_iterator it = mesh.verts().begin(); it != mesh.verts().end(); ) {
		const double x = *(it++);
		const double y = *(it++);
		const double z = *(it++);
		obj_stream << "v " << x << " " << y << " " << z << "\n";
	}

	for ( std::vector<double>::const_iterator it = mesh.normals().begin(); it != mesh.normals().end(); ) {
		const double x = *(it++);
		const double y = *(it++);
		const double z = *(it++);
		obj_stream << "vn " << x << " " << y << " " << z << "\n";
	}

	int previous_material_id = -2;
	std::vector<int>::const_iterator material_it = mesh.material_ids().begin();

	for ( std::vector<int>::const_iterator it = mesh.faces().begin(); it != mesh.faces().end(); ) {
		
		const int material_id = *(material_it++);
		if (material_id != previous_material_id) {
			const IfcGeom::Material& material = mesh.materials()[material_id];
			const std::string material_name = material.name();
			obj_stream << "usemtl " << material_name << "\n";
			if (materials.find(material_name) == materials.end()) {
				writeMaterial(material);
				materials.insert(material_name);
			}
			previous_material_id = material_id;
		}

		const int v1 = *(it++)+vcount_total;
		const int v2 = *(it++)+vcount_total;
		const int v3 = *(it++)+vcount_total;
		obj_stream << "f " << v1 << "//" << v1 << " " << v2 << "//" << v2 << " " << v3 << "//" << v3 << "\n";

	}

	std::set<int> faces_set (mesh.faces().begin(), mesh.faces().end());
	const std::vector<int>& edges = mesh.edges();

	for ( std::vector<int>::const_iterator it = edges.begin(); it != edges.end(); ) {
		const int i1 = *(it++);
		const int i2 = *(it++);

		if (faces_set.find(i1) != faces_set.end() || faces_set.find(i2) != faces_set.end()) {
			continue;
		}

		const int material_id = *(material_it++);

		if (material_id != previous_material_id) {
			const IfcGeom::Material& material = mesh.materials()[material_id];
			const std::string material_name = material.name();
			obj_stream << "usemtl " << material_name << "\n";
			if (materials.find(material_name) == materials.end()) {
				writeMaterial(material);
				materials.insert(material_name);
			}
			previous_material_id = material_id;
		}

		const int v1 = i1 + vcount_total;
		const int v2 = i2 + vcount_total;

		obj_stream << "l " << v1 << " " << v2 << "\n";
	}

	vcount_total += vcount;
}
