#pragma once

#include "lib_define.hpp"
#include "lib_func_registry.hpp"
#include "lib_context.hpp"
#include <vector>

namespace Lua {
    namespace Lib {

        /**
         * Modern library module interface compatible with existing codebase
         */
        class LibModule {
        public:
            virtual ~LibModule() = default;
            
            /**
             * Get module name
             */
            virtual StrView getName() const noexcept = 0;
            
            /**
             * Get module version
             */
            virtual StrView getVersion() const noexcept { return "1.0"; }
            
            /**
             * Register module functions
             */
            virtual void registerFunctions(LibFuncRegistry& registry, const LibContext& context) = 0;
            
            /**
             * Initialize module (called after registration)
             */
            virtual void initialize(State* /*state*/, const LibContext& /*context*/) {}
            
            /**
             * Cleanup module resources
             */
            virtual void cleanup(State* /*state*/, const LibContext& /*context*/) {}
            
            /**
             * Check if module has dependencies
             */
            virtual std::vector<StrView> getDependencies() const { return {}; }
            
            /**
             * Module configuration
             */
            virtual void configure(LibContext& /*context*/) {}
        };

        /**
         * Module registration helper
         */
        template<typename ModuleType>
        std::unique_ptr<LibModule> createModule() {
            static_assert(std::is_base_of_v<LibModule, ModuleType>, 
                         "ModuleType must inherit from LibModule");
            return std::make_unique<ModuleType>();
        }

    } // namespace Lib
} // namespace Lua