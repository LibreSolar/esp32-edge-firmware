const CompressionPlugin = require("compression-webpack-plugin");

module.exports = {
  productionSourceMap: false,
  devServer: {
    proxy: {
      '/ts': {
        // add IP address of device below for frontend testing within local network
        target: 'http://',
        changeOrigin: true,
        ws: true
      },
      '/ota': {
        target: 'http://',
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
      deleteOriginalAssets: process.env.NODE_ENV === 'production' ? true : false,
    })]
  }
}
