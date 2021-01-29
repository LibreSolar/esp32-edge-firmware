import Vue from 'vue'
import Router from 'vue-router'
import Home from './views/Home.vue'
import Live from './views/Live.vue'
import Config from './views/Config.vue'
import Ota from './views/Ota.vue'

Vue.use(Router)

export default new Router({
  mode: 'history',
  base: process.env.BASE_URL,
  routes: [
    {
      path: '/',
      name: 'home',
      component: Home
    },
    {
      path: '/live',
      name: 'live',
      component: Live
    },
    {
      path: '/config',
      name: 'config',
      component: Config
    },
    {
      path: '/ota',
      name: 'ota',
      component: Ota
    }
  ]
})
