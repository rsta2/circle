// typedoc.js
/**
 * @type {import('typedoc').TypeDocOptions}
 */
module.exports = {
  entryPoints: ['src/superlog.ts'],
  out: 'docs',
  plugin: ['typedoc-plugin-markdown'],
  entryDocument: 'API.md',
  readme: 'src/doc/API.md',
  // publicPath: '.',
  hideBreadcrumbs: false,
  preserveAnchorCasing: true,
  excludeExternals: false,
  excludeNotDocumented: false,
  excludeInternal: true,
  excludePrivate: true,
  excludeProtected: true,
  sort: ['source-order', 'required-first'],
};
