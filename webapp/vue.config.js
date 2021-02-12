const CompressionPlugin = require("compression-webpack-plugin");

module.exports = {
  productionSourceMap: false,
  devServer: {
    proxy: {
      '/api/v1/ts': {
        target: 'http://192.168.0.61',
        changeOrigin: true,
        ws: true
      }
    }
  },
  chainWebpack: (config) => {
    config.optimization.delete('splitChunks'),
    config.module
      .rule('images')
      .use('url-loader')
      .tap(options => Object.assign({}, options, { name: '[name].[ext]' }));
  },
  css: {
    extract: {
      filename: '[name].css',
      chunkFilename: '[name].css',
    },
  },
  configureWebpack: {
    output: {
      filename: '[name].js',
      chunkFilename: '[name].js',
    },
    plugins: [new CompressionPlugin({
      filename: '[name][ext].gz',
      algorithm: "gzip",
      deleteOriginalAssets: false,
    })]
  }
}
