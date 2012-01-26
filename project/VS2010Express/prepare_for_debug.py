import sys
import os
import shutil

def copy_required_files(exe_dir_path, project_root_path="../.."):
	def new_or_modified_file_callback_for_depends(src_file_path):
		new_dst_dir_path = os.path.dirname(src_file_path).replace(depends_path, exe_dir_path)	# Notice that depends_path and exe_dir_path are context-dependent.
		print "Copying: %s -> %s" % (src_file_path, new_dst_dir_path)
		shutil.copy2(src_file_path, new_dst_dir_path)	# Use copy2() to preserve the modification time.
		
	def new_or_modified_file_callback_for_subdirs(src_file_path):
		new_dst_dir_path = os.path.dirname(src_file_path).replace(project_root_path, exe_dir_path)	# Notice that project_root_path and exe_dir_path are context-dependent.
		print "Copying: %s -> %s" % (src_file_path, new_dst_dir_path)
		shutil.copy2(src_file_path, new_dst_dir_path)	# Use copy2() to preserve the modification time.
	
	def new_or_modified_file_callback_for_qt(src_file_path):
		new_dst_dir_path = os.path.dirname(src_file_path).replace(project_root_path, exe_dir_path).replace("qt", "players").replace("win32_release", "flashplayer")	# Notice that project_root_path and exe_dir_path are context-dependent.
		print "Copying: %s -> %s" % (src_file_path, new_dst_dir_path)
		shutil.copy2(src_file_path, new_dst_dir_path)	# Use copy2() to preserve the modification time.
		
	def new_or_modified_file_callback_for_qt_plugins(src_file_path):
		new_dst_dir_path = os.path.dirname(src_file_path).replace(project_root_path, exe_dir_path).replace("win32_release", "win32")	# Notice that project_root_path and exe_dir_path are context-dependent.
		print "Copying: %s -> %s" % (src_file_path, new_dst_dir_path)
		shutil.copy2(src_file_path, new_dst_dir_path)	# Use copy2() to preserve the modification time.
		
	project_root_path = os.path.abspath(project_root_path)
	exe_dir_path = os.path.abspath(exe_dir_path)
	
	check_files_module_path = os.path.join(project_root_path, "misc")
	sys.path.append(check_files_module_path)
	from check_files import find_changes
	
	dirs_to_copy = ("language", "media", "scripts", "skin", "system", "visualisations")
	for dir_to_copy in dirs_to_copy:
		src_dir_path = os.path.join(project_root_path, dir_to_copy)
		dst_dir_path = os.path.join(exe_dir_path, dir_to_copy)
		if not os.path.exists(dst_dir_path):
			print "Copying: %s -> %s" % (src_dir_path, dst_dir_path)
			shutil.copytree(src_dir_path, dst_dir_path)
		else:
			find_changes(src_dir_path, new_or_modified_file_callback_for_subdirs, new_or_modified_file_callback_for_subdirs, None)
		if dir_to_copy == "system":
			src_dir_path = os.path.join(src_dir_path, "qt/win32_release")
			dst_dir_path = src_dir_path.replace(project_root_path, exe_dir_path).replace("qt", "players").replace("win32_release", "flashplayer")
			if not os.path.exists(dst_dir_path):
				print "Copying: %s -> %s" % (src_dir_path, dst_dir_path)
				shutil.copytree(src_dir_path, dst_dir_path)
			else:
				find_changes(src_dir_path, new_or_modified_file_callback_for_qt, new_or_modified_file_callback_for_qt, None)
			src_dir_path = os.path.join(src_dir_path, "plugins")
			dst_dir_path = src_dir_path.replace(project_root_path, exe_dir_path).replace("win32_release", "win32")
			if not os.path.exists(os.path.join(dst_dir_path,"..")):
				os.mkdir(os.path.join(dst_dir_path,".."))
			if not os.path.exists(dst_dir_path):
				print "Copying: %s -> %s" % (src_dir_path, dst_dir_path)
				shutil.copytree(src_dir_path, dst_dir_path)
			else:
				find_changes(src_dir_path, new_or_modified_file_callback_for_qt_plugins, new_or_modified_file_callback_for_qt_plugins, None)
	depends_path = os.path.abspath(os.path.join(project_root_path, "project/Win32BuildSetup/dependencies"))
	find_changes(depends_path, new_or_modified_file_callback_for_depends, new_or_modified_file_callback_for_depends, None)

		
if __name__ == '__main__':
	if len(sys.argv) < 2:
		print "Usage: %s <exe_dir_path> [<project_root_path>]" % (sys.argv[0],)
		sys.exit(1)
	copy_required_files(*sys.argv[1:])
