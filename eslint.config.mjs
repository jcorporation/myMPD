import globals from "globals";
import pluginJs from "@eslint/js";
import pluginJsdoc from "eslint-plugin-jsdoc";

export default [
    pluginJs.configs.recommended,
    {
        files: [
            "htdocs/sw.js",
            "release/htdocs/js/mympd.js"
        ],
        plugins: {
            jsdoc: pluginJsdoc
        },
        linterOptions: {
            reportUnusedDisableDirectives: "warn"
        },
        languageOptions: {
            sourceType: "script",
            globals: {
                ...globals.browser,
                "Atomics": "readonly",
                "SharedArrayBuffer": "readonly",
                "BSN": true,
                "i18n": true,
                "MediaMetadata": true
            }
        },
        rules: {
            "block-scoped-var": "error",
            "camelcase": "error",
            "default-case": "error",
            "default-case-last": "error",
            "eqeqeq": [ "error", "always", {
                "null": "ignore"
            }],
            "semi": [ 2, "always"],
            "no-alert": "error",
            "no-caller": "error",
            "no-console": "off",
            "no-eq-null": "error",
            "no-eval": "error",
            "no-implied-eval": "error",
            "no-invalid-this": "error",
            "no-restricted-globals": "error",
            "no-restricted-properties": "error",
            "no-return-assign": "error",
            "no-self-compare": "error",
            "no-shadow": "error",
            "no-shadow-restricted-names": "error",
            "no-useless-concat": "error",
            "no-var": "error",
            "prefer-const": "error",
            "jsdoc/check-syntax": 1,
            "jsdoc/newline-after-description": "off",
            "jsdoc/no-blank-block-descriptions": 1,
            "jsdoc/no-defaults": 1,
            "jsdoc/no-undefined-types": [ "warn", {
                "definedTypes": [
                    "ChildNode",
                    "ParentNode"
                ]
            }],
            "jsdoc/require-returns": [ "error", {
                "forceRequireReturn": true
            }]
        }
    },
    {
        files: [
            "release/htdocs/sw.min.js",
            "release/htdocs/js/mympd.min.js",
            "release/htdocs/js/i18n.min.js"
        ],
        linterOptions: {
            reportUnusedDisableDirectives: "warn"
        },
        languageOptions: {
            sourceType: "script",
            globals: {
                ...globals.browser,
                "Atomics": "readonly",
                "SharedArrayBuffer": "readonly",
                "BSN": true,
                "i18n": true,
                "MediaMetadata": true
            },
        },
        rules: {
            "block-scoped-var": "error",
            "camelcase": "error",
            "default-case-last": "error",
            "eqeqeq": [ "error", "always", {
                "null": "ignore"
            }],
            "no-alert": "error",
            "no-caller": "error",
            "no-console": "off",
            "no-empty": "off",
            "no-eq-null": "error",
            "no-eval": "error",
            "no-fallthrough": "off",
            "no-implied-eval": "error",
            "no-invalid-this": "error",
            "no-redeclare": [ "error", {
                "builtinGlobals": false
            }],
            "no-restricted-globals": "error",
            "no-restricted-properties": "error",
            "no-return-assign": "error",
            "no-self-compare": "error",
            "no-shadow": "error",
            "no-shadow-restricted-names": "error",
            "no-unused-vars": "off",
            "no-useless-concat": "off",
            "no-var": "error",
            "prefer-const": "error"
        }
    }
];
