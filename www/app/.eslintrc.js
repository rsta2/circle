module.exports = {
  env: {
    browser: true,
    node: true,
    es2021: true,
  },
  extends: ['eslint:recommended', 'plugin:import/recommended', 'plugin:prettier/recommended', 'prettier'],
  parserOptions: {
    ecmaFeatures: {
      jsx: false,
    },
    ecmaVersion: 2018,
    sourceType: 'module',
  },
  plugins: ['prettier', 'json', 'import'],
  settings: {
    react: { version: '16' },
  },
  rules: {
    'no-unused-vars': [1, { vars: 'all', args: 'none' }],
    'import/no-unresolved': 'error',
    'import/order': [
      'error',
      {
        groups: ['builtin', 'external', 'internal', 'parent', 'sibling', 'index', 'object', 'type'],
        alphabetize: {
          order: 'asc' /* sort in ascending order. Options: ['ignore', 'asc', 'desc'] */,
          // orderImportKind: 'asc',
          caseInsensitive: false /* ignore case. Options: [true, false] */,
        },
        'newlines-between': 'always', // clashes with prettier
        warnOnUnassignedImports: true,
      },
    ],
    'import/no-default-export': 'error',
    'import/namespace': [0],
  },
  overrides: [
    {
      files: ['*.ts', '*.tsx'],
      extends: [
        // 'eslint:recommended',
        'plugin:@typescript-eslint/recommended',
        'plugin:import/typescript',
        // 'prettier/@typescript-eslint',
        // 'plugin:prettier/recommended',
        // 'plugin:react/recommended',
        // 'prettier',
        // 'prettier/react',
        // 'plugin:jsx-a11y/recommended',
        // 'plugin:react-hooks/recommended',
      ],
      parser: '@typescript-eslint/parser',
      parserOptions: {
        ecmaFeatures: {
          project: ['./tsconfig.json'],
        },
      },
      rules: {
        '@typescript-eslint/no-unused-vars': [1, { vars: 'all', args: 'none' }],
        '@typescript-eslint/no-empty-interface': 0,
        '@typescript-eslint/no-inferrable-types': [
          1,
          {
            ignoreParameters: true,
            ignoreProperties: true,
          },
        ],
        '@typescript-eslint/ban-types': [
          2,
          {
            extendDefaults: true,
            types: {
              '{}': false,
            },
          },
        ],
      },
    },
    // {
    //   files: ['*.stories.tsx'],
    //   rules: {
    //     '@typescript-eslint/no-unused-vars': [2, { vars: 'all', args: 'none' }],
    //     '@typescript-eslint/no-empty-interface': 0,
    //   },
    // },
  ],
};
