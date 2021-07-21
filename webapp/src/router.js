import Vue from 'vue'
import Router from 'vue-router'
import store from './store'
import Info from './views/Info.vue'
import Home from './views/Home.vue'
import Live from './views/Live.vue'
import Config from './views/Config.vue'
import IO from './views/IO.vue'
import Ota from './views/Ota.vue'
import DataLog from './views/DataLog.vue'
import ConfigGeneral from './views/ConfigGeneral.vue'

Vue.use(Router)

const router = new Router({
  mode: 'abstract',
  base: process.env.BASE_URL,
  routes: [
    {
      path: '/',
      name: 'home',
      component: Home
    },
    {
      path: '/info',
      name: 'info',
      component: Info
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
      path: '/io',
      name: 'io',
      component: IO
    },
    {
      path: '/data-log',
      name: 'data-log',
      component: DataLog
    },
    {
      path: '/ota',
      name: 'ota',
      component: Ota
    },
    {
      path: '/esp-config',
      name: 'esp-config',
      component: ConfigGeneral
    }
  ]
})

router.beforeEach((to, from, next) => {
  if (!(['esp-config', 'home'].includes(to.name)) && store.state.loading) {
    next(false)
  }
  else next()
})

export default router
