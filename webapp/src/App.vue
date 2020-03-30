<template>
  <v-app id="inspire">
    <v-navigation-drawer v-model="drawer" fixed app clipped>
      <v-list>
        <v-list-tile to="/">
          <v-list-tile-action>
            <v-icon>home</v-icon>
          </v-list-tile-action>
          <v-list-tile-content>
            <v-list-tile-title>Device Info</v-list-tile-title>
          </v-list-tile-content>
        </v-list-tile>
        <v-list-tile to="/live">
          <v-list-tile-action>
            <v-icon>timeline</v-icon>
          </v-list-tile-action>
          <v-list-tile-content>
            <v-list-tile-title>Live View</v-list-tile-title>
          </v-list-tile-content>
        </v-list-tile>
        <v-list-tile to="/config">
          <v-list-tile-action>
            <v-icon>tune</v-icon>
          </v-list-tile-action>
          <v-list-tile-content>
            <v-list-tile-title>Configuration</v-list-tile-title>
          </v-list-tile-content>
        </v-list-tile>
        <v-list-tile to="/inout">
          <v-list-tile-action>
            <v-icon>sync_alt</v-icon>
          </v-list-tile-action>
          <v-list-tile-content>
            <v-list-tile-title>Input/Output</v-list-tile-title>
          </v-list-tile-content>
        </v-list-tile>
        <v-list-tile to="/log">
          <v-list-tile-action>
            <v-icon>bar_chart</v-icon>
          </v-list-tile-action>
          <v-list-tile-content>
            <v-list-tile-title>Data Log</v-list-tile-title>
          </v-list-tile-content>
        </v-list-tile>
      </v-list>
    </v-navigation-drawer>
    <v-toolbar color="primary" dark fixed app clipped-left>
      <v-toolbar-side-icon @click.stop="drawer = !drawer"></v-toolbar-side-icon>
      <v-spacer></v-spacer>
      <v-menu :nudge-width="100">
        <v-toolbar-title slot="activator">
          <span>MPPT 1210 HUS</span>
          <v-icon dark>arrow_drop_down</v-icon>
        </v-toolbar-title>
        <v-list>
          <v-list-tile v-for="item in items" :key="item" @click=";">
            <v-list-tile-title v-text="item"></v-list-tile-title>
          </v-list-tile>
        </v-list>
      </v-menu>

      <v-spacer></v-spacer>
      <v-btn icon>
        <v-icon>settings</v-icon>
      </v-btn>
    </v-toolbar>
    <v-content>
      <v-container fluid fill-height>
        <router-view></router-view>
      </v-container>
    </v-content>
    <v-footer color="primary">
      <v-spacer></v-spacer>
      <span class="white--text">Made with <v-icon small dark>favorite_border</v-icon> by Libre Solar</span>
      <v-spacer></v-spacer>
    </v-footer>
  </v-app>
</template>

<script>
export default {
  name: 'App',
  data() {
    return {
      drawer: null,
      items: [
        'MPPT 1210 HUS', 'BMS 15S80 SC', 'MPPT 2420 LC'
      ]
    }
  }
}
</script>
