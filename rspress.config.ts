import { defineConfig } from '@rspress/core';
import mermaid from 'rspress-plugin-mermaid';

export default defineConfig({
  plugins: [mermaid()],
  root: 'docs',
  base: '/GodotMCP-GDExtension/',
  lang: 'zh',
  locales: [
    {
      lang: 'zh',
      label: '🇨🇳 简体中文',
      title: 'GodotMCP',
      description: '通过 Model Context Protocol 将 Godot 4.6+ 编辑器接入 AI 编码助手',
    },
    {
      lang: 'en',
      label: '🇺🇸 English',
      title: 'GodotMCP',
      description: 'Bridge between AI coding assistants and the Godot 4.6+ editor via the Model Context Protocol',
    },
  ],
  icon: '/favicon.svg',
  logoText: 'GodotMCP',
  outDir: 'doc_build',
  nav: [
    { text: 'about', link: '/about/' },
    { text: 'guide', link: '/guide/' },
    { text: 'reference', link: '/reference/' },
    { text: 'api', link: '/api/' },
    { text: 'changelog', link: '/changelog/' },
  ],
  themeConfig: {
    sidebar: {
      '/': [
        {
          text: 'about',
          collapsible: true,
          items: [
            { text: 'overview', link: '/about/overview' },
            { text: 'architecture', link: '/about/architecture' },
            { text: 'technicalDetails', link: '/about/technical-details' },
            { text: 'projectStructure', link: '/about/project-structure' },
          ],
        },
        {
          text: 'guide',
          collapsible: true,
          items: [
            { text: 'gettingStarted', link: '/guide/getting-started' },
            { text: 'configuration', link: '/guide/configuration' },
            { text: 'clientSetup', link: '/guide/client-setup' },
            { text: 'building', link: '/guide/building' },
            { text: 'toolsOverview', link: '/guide/tools-overview' },
            { text: 'faq', link: '/guide/faq' },
          ],
        },
        {
          text: 'reference',
          collapsible: true,
          items: [
            { text: 'metaTools', link: '/reference/meta-tools' },
            { text: 'sceneTreeTools', link: '/reference/scene-tree-tools' },
            { text: 'scriptTools', link: '/reference/script-tools' },
            { text: 'filesystemTools', link: '/reference/filesystem-tools' },
            { text: 'workspaceTools', link: '/reference/workspace-tools' },
            { text: 'animationTools', link: '/reference/animation-tools' },
            { text: 'runtimeTools', link: '/reference/runtime-tools' },
            { text: 'resourceTools', link: '/reference/resource-tools' },
            { text: 'signalGroupTools', link: '/reference/signal-group-tools' },
            { text: 'controlCollisionTools', link: '/reference/control-collision-tools' },
            { text: 'audioNav3dTilemapTools', link: '/reference/audio-nav-3d-tilemap' },
            { text: 'shaderTools', link: '/reference/shader-tools' },
            { text: 'settingsExportTools', link: '/reference/settings-export-tools' },
            { text: 'docTools', link: '/reference/doc-tools' },
          ],
        },
        {
          text: 'api',
          collapsible: true,
          items: [
            { text: 'mcpToolDefinition', link: '/api/mcp-tool-definition' },
            { text: 'mcpToolRegistry', link: '/api/mcp-tool-registry' },
            { text: 'handlerRegistry', link: '/api/handler-registry' },
            { text: 'itoolInterface', link: '/api/itool-interface' },
            { text: 'cmdUtils', link: '/api/cmd-utils' },
            {
              text: 'testing',
              items: [
                { text: 'overview', link: '/api/testing/overview' },
                { text: 'pipelineFormat', link: '/api/testing/pipeline-format' },
                { text: 'assertionSystem', link: '/api/testing/assertion-system' },
                { text: 'orchestrator', link: '/api/testing/orchestrator' },
                { text: 'writingTests', link: '/api/testing/writing-tests' },
              ],
            },
          ],
        },
        {
          text: 'changelog',
          collapsible: true,
          items: [
            { text: 'overview', link: '/changelog/' },
            { text: 'v0.2.2', link: '/changelog/v0-2-2' },
            { text: 'v0.2.1', link: '/changelog/v0-2-1' },
            { text: 'v0.2.0', link: '/changelog/v0-2-0' },
            { text: 'v0.1.5', link: '/changelog/v0-1-5' },
            { text: 'v0.1.4', link: '/changelog/v0-1-4' },
            { text: 'v0.1.3', link: '/changelog/v0-1-3' },
            { text: 'v0.1.2', link: '/changelog/v0-1-2' },
            { text: 'v0.1.1', link: '/changelog/v0-1-1' },
            { text: 'v0.1.0', link: '/changelog/v0-1-0' },
          ],
        },
      ],
    },
  },
});
