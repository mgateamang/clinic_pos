project('clinic_pos', 'cpp',
  version : '1.0.0',
  default_options : ['warning_level=2', 'cpp_std=c++17']
)

qt6 = import('qt6')
qt6_dep = dependency('qt6', modules : ['Core', 'Gui', 'Qml', 'Quick', 'Network'], required : true)

inc = include_directories('include')

# Preprocess MOC and QRC resources for main executable
qt_files = qt6.preprocess(
  moc_headers : ['include/pos_controller.hpp'],
  qresources : ['qrc/qml.qrc']
)

executable('clinic_pos',
  files('src/main.cpp', 'src/pos_controller.cpp') + qt_files,
  include_directories : inc,
  dependencies : qt6_dep,
  install : true
)

# Test runner (only needs MOC, no GUI window display required)
test_moc = qt6.preprocess(
  moc_headers : ['include/pos_controller.hpp']
)

test_exec = executable('clinic_pos_test',
  files('tests/test_pos.cpp', 'src/pos_controller.cpp') + test_moc,
  include_directories : inc,
  dependencies : qt6_dep
)

test('pos_tests', test_exec)
