# Copyright 2019-2020 CERN and copyright holders of ALICE O2.
# See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
# All rights not expressly granted are reserved.
#
# This software is distributed under the terms of the GNU General Public
# License v3 (GPL Version 3), copied verbatim in the file "COPYING".
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

# FIXME: the LinkDef should not be in the public area

o2_add_library(Mergers
               SOURCES src/MergerAlgorithm.cxx src/IntegratingMerger.cxx src/MergerInfrastructureBuilder.cxx
                       src/MergerBuilder.cxx src/FullHistoryMerger.cxx src/ObjectStore.cxx
               PUBLIC_LINK_LIBRARIES O2::Framework AliceO2::InfoLogger)

o2_target_root_dictionary(
  Mergers
  HEADERS include/Mergers/MergeInterface.h
  include/Mergers/CustomMergeableObject.h
          include/Mergers/CustomMergeableTObject.h
  LINKDEF include/Mergers/LinkDef.h)

o2_add_executable(benchmark-topology
                  SOURCES test/mergersBenchmarkTopology.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers)

o2_add_executable(multinode-benchmark-producers
                  SOURCES test/multinodeBenchmarkProducers.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers)

o2_add_executable(multinode-benchmark-mergers
                  SOURCES test/multinodeBenchmarkMergers.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers)

o2_add_executable(benchmark-empty-loop
                  SOURCES test/emptyLoopBenchmark.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers AliceO2::Common)

if (TARGET benchmark::benchmark)
o2_add_executable(benchmark-miscellaneous
                  SOURCES test/benchmark_Miscellaneous.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers benchmark::benchmark)

o2_add_executable(benchmark-merging-collections
                  SOURCES test/benchmark_MergingCollections.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers benchmark::benchmark)

o2_add_executable(benchmark-full-vs-diff
                  SOURCES test/benchmark_FullVsDiff.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers benchmark::benchmark)

o2_add_executable(benchmark-types
                  SOURCES test/benchmark_Types.cxx
                  COMPONENT_NAME mergers
                  PUBLIC_LINK_LIBRARIES O2::Mergers Boost::serialization benchmark::benchmark)
endif()

o2_add_test(InfrastructureBuilder
            SOURCES test/test_InfrastructureBuilder.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(Algorithm
            SOURCES test/test_Algorithm.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(ObjectStore
            SOURCES test/test_ObjectStore.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyHistosIntegrating
            SOURCES test/test_MergerTopologyHistosIntegrating.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyHistosFullHistory
            SOURCES test/test_MergerTopologyHistosFullHistory.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyCustomIntegrating
            SOURCES test/test_MergerTopologyCustomIntegrating.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyCustomFullHistory
            SOURCES test/test_MergerTopologyCustomFullHistory.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyVectorIntegrating
            SOURCES test/test_MergerTopologyVectorIntegrating.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)

o2_add_test(TopologyVectorFullHistory
            SOURCES test/test_MergerTopologyVectorFullHistory.cxx
            COMPONENT_NAME mergers
            PUBLIC_LINK_LIBRARIES O2::Mergers
            LABELS utils)
