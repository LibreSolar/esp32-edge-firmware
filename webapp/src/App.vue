<template>
  <v-app>
    <v-app-bar
      app
      color="primary"
      clipped-left
      dark
    >
      <v-app-bar-nav-icon @click.stop="drawer = !drawer"></v-app-bar-nav-icon>
      <v-spacer></v-spacer>
      <v-menu offset-y>
        <template v-slot:activator=" {on, attrs}">
          <v-btn color="secondary" dark v-bind="attrs" v-on="on">
            {{ $store.state.activeDevice }}
          </v-btn>
        </template>
        <v-list>
          <v-list-item
          v-for="key in Object.keys($store.state.devices)"
          :key="key"
          @click="store.commit('changeDevice', key)">
            <v-list-item-title v-text="key"></v-list-item-title>
          </v-list-item>
        </v-list>
      </v-menu>
      <v-spacer></v-spacer>
      <v-btn
        href="https://github.com/LibreSolar/data-manager-firmware"
        target="_blank"
        text
      >
        <span class="mr-2" v-if="$vuetify.breakpoint.mdAndUp">Latest Release</span>
        <v-icon>mdi-open-in-new</v-icon>
      </v-btn>
    </v-app-bar>

    <v-main class="accent" v-if="!$store.state.loading">
        <router-view/>
    </v-main>

    <v-navigation-drawer v-model="drawer" app clipped :expand-on-hover="$vuetify.breakpoint.lgAndUp">
      <v-list dense nav>
        <v-list-item
          v-for="item in items"
          :key="item.title"
          link :to="item.href"
        >
          <v-list-item-icon>
            <v-icon>{{ item.icon }}</v-icon>
          </v-list-item-icon>

          <v-list-item-content>
            <v-list-item-title>{{ item.title }}</v-list-item-title>
          </v-list-item-content>
        </v-list-item>
      </v-list>
    </v-navigation-drawer>

    <v-footer color="primary">
      <v-spacer></v-spacer>
      <span class="white--text">Made with <v-icon small dark>mdi-heart</v-icon> by Libre Solar</span>
      <v-spacer></v-spacer>
    </v-footer>
  </v-app>
</template>

<script>


export default {
  name: 'App',
  created() {
    this.$store.dispatch('getDevices')
  },
  data () {
    return {
      drawer: null,
      items: [
        { title: 'Device Info', href: '/', icon: 'mdi-home' },
        { title: 'Live View', href: '/live', icon: 'mdi-chart-line' },
        { title: 'Configuration', href: '/config', icon: 'mdi-tune' },
        { title: 'Input/Output', href: '/io', icon: 'mdi-swap-horizontal' },
        { title: 'Data Log', href: '/data-log', icon: 'mdi-chart-bar' },
        { title: 'Firmware Upgrade', href: '/ota', icon: 'mdi-upload'}
      ],
      right: null,
      dialog: false
    }
  }
};
</script>